from types import SimpleNamespace as N
from concurrent.futures import ThreadPoolExecutor
from unittest.mock import Mock
import pytest
from pydantic import ValidationError
from libracore_api.config import Settings
from libracore_api import schemas as s
from libracore_api.ai.providers import (ToolCall,LLMResponse,MockLLMProvider,GeminiLLMProvider,OpenAILLMProvider,gemini_schema)
from libracore_api.ai.tools import declarations, validate_call,TOOLS
from libracore_api.ai.assistant import Assistant
from libracore_api.ai.confirmation import Confirmations
from libracore_api.services import DomainError

@pytest.mark.parametrize('provider,openai,gemini,expected',[
 ('auto','a','b','openai'),('auto','','b','gemini'),('auto','','','mock'),
 ('mock','a','b','mock'),('gemini','a','b','gemini'),('openai','a','b','openai')])
def test_selection(provider,openai,gemini,expected):
    cfg=Settings(_env_file=None,llm_provider=provider,openai_api_key=openai,gemini_api_key=gemini,
                 openai_model='test-model',gemini_model='test-model')
    assert cfg.provider==expected
    assert openai not in repr(cfg.openai_api_key) if openai else True

@pytest.mark.parametrize('provider',['gemini','openai'])
def test_missing_credentials(provider):
    with pytest.raises(ValidationError):Settings(_env_file=None,llm_provider=provider,openai_api_key='',gemini_api_key='')

def test_config_flags_and_urls(monkeypatch):
    monkeypatch.setenv('AI_ENABLED','false')
    c=Settings(_env_file=None,postgres_password='a:@b',test_database_url='')
    assert not c.ai_enabled and 'a%3A%40b' in c.db_url()
    assert c.db_url(True).endswith('/libracore_test')
    with pytest.raises(ValueError):Settings(_env_file=None,ai_enabled=False,test_database_url='postgresql://localhost/libracore').db_url(True)
    with pytest.raises(ValidationError):Settings(_env_file=None,llm_fallback_on_error=True)

@pytest.mark.parametrize('message,name,args',[
 ('Find books by Scott Meyers','search_resources',{'author':'Scott Meyers','category':0}),
 ('How many copies of B001 are available?','get_availability',{'resource_id':'B001'}),
 ('Can Computer Science buy 3 copies of B001?','check_purchase',{'department_id':'Computer Science','quantity':3,'resource_id':'B001'}),
 ('Borrow B001 for U001','borrow_resource',{'resource_id':'B001','borrower_id':'U001','quantity':1}),
 ('Cancel order 25','cancel_purchase',{'order_id':25})])
def test_mock(message,name,args):assert MockLLMProvider().interpret(message,[]).calls==[ToolCall(name,args)]

def test_sdk_normalization(settings):
    args={'resource_id':'B001'}
    gc=Mock();gc.models.generate_content.return_value=N(function_calls=[N(name='get_resource',args=args)])
    oc=Mock();oc.chat.completions.create.return_value=N(choices=[N(message=N(tool_calls=[N(function=N(name='get_resource',arguments='{"resource_id":"B001"}'))]))])
    a=GeminiLLMProvider(settings,gc).interpret('read',declarations())
    b=OpenAILLMProvider(settings,oc).interpret('read',declarations())
    assert a.calls==b.calls==[ToolCall('get_resource',args)]
    config=gc.models.generate_content.call_args.kwargs['config']
    assert config.automatic_function_calling.disable
    assert oc.chat.completions.create.call_args.kwargs['parallel_tool_calls'] is False

def test_all_gemini_schemas():
    from google.genai import types
    for d in declarations():types.Schema(**gemini_schema(d['parameters']))

@pytest.mark.parametrize('provider,method',[(GeminiLLMProvider,'gemini'),(OpenAILLMProvider,'openai')])
def test_provider_error_redacted(settings,provider,method):
    c=Mock()
    if method=='gemini':c.models.generate_content.side_effect=RuntimeError('secret-key')
    else:c.chat.completions.create.side_effect=RuntimeError('secret-key')
    with pytest.raises(DomainError) as e:provider(settings,c).interpret('hello',declarations())
    assert e.value.code=='PROVIDER_ERROR' and 'secret' not in str(e.value)

@pytest.mark.parametrize('call',[ToolCall('DROP TABLE',{}),ToolCall('borrow_resource',{'resource_id':'B','borrower_id':'U','quantity':0}),ToolCall('cancel_purchase',{'order_id':1,'request_id':'chosen'}),ToolCall('cancel_purchase',{'order_id':1,'confirmed':True})])
def test_bad_tools(call):
    with pytest.raises(DomainError):validate_call(call)

def test_registry_complete():
    assert len(TOOLS)==14
    for d in declarations():assert 'request_id' not in d['parameters']['properties']

def test_confirmation_single_use_and_binding():
    c=Confirmations();args={'order_id':1};p=c.propose('s','cancel_purchase',args);args['order_id']=2
    with pytest.raises(DomainError):c.consume(p['token'],'other')
    assert c.consume(p['token'],'s')==('cancel_purchase',{'order_id':1})
    with pytest.raises(DomainError):c.consume(p['token'],'s')

def test_confirmation_expiry():
    c=Confirmations(ttl=-1);p=c.propose('s','cancel_purchase',{})
    with pytest.raises(DomainError):c.consume(p['token'],'s')

def test_confirmation_concurrent():
    c=Confirmations();p=c.propose('s','cancel_purchase',{})
    def use(_):
        try:c.consume(p['token'],'s');return True
        except DomainError:return False
    with ThreadPoolExecutor(2) as pool:assert sum(pool.map(use,range(2)))==1

def test_assistant_confirmation(settings):
    service=Mock();service.execute.return_value={'order_id':25,'quantity':1,'status':3,'approved':True,'cost':{'minor_units':100,'currency':'INR','formatted':'INR 1.00'}}
    a=Assistant(settings,service);r=a.message('Cancel order 25','session')
    service.execute.assert_not_called()
    a.confirm(r['confirmations'][0]['token'],'session')
    service.execute.assert_called_once()
    assert service.execute.call_args.args[1]['order_id']==25

def test_disabled_write(settings):
    settings.ai_write_actions_enabled=False
    service=Mock();a=Assistant(settings,service)
    with pytest.raises(DomainError):a.message('Borrow B001 for U001','s')
    service.execute.assert_not_called()

def test_no_unsupported_claims(settings):
    service=Mock();r=Assistant(settings,service).message('make up a budget','s')
    service.execute.assert_not_called();assert not r['tool_results']

@pytest.mark.parametrize('data',[{'quantity':0},{'quantity':-1},{'quantity':True},{'quantity':1.5}])
def test_quantity_validation(data):
    with pytest.raises(ValidationError):s.Purchase(resource_id='B',**data)

def test_dates():
    with pytest.raises(ValidationError):s.Borrow(resource_id='B',borrower_id='U',request_id='x',due_at='2026-01-01T00:00:00')
    b=s.Borrow(resource_id='B',borrower_id='U',request_id='x',due_at='2026-01-01T05:30:00+05:30')
    assert b.due_at.hour==0

def test_typed_output_rejects_invented_payload():
    from libracore_api.ai.tools import validate_output
    with pytest.raises(DomainError):validate_output('get_availability',{'available':'invented'})

def test_missing_model_is_configuration_failure():
    with pytest.raises(ValidationError):Settings(_env_file=None,llm_provider='gemini',gemini_api_key='fake',gemini_model='')

def test_multiple_tool_calls_rejected_before_execution(settings):
    provider=Mock();provider.interpret.return_value=LLMResponse([ToolCall('get_resource',{'resource_id':'B'}),ToolCall('cancel_purchase',{'order_id':1})])
    service=Mock()
    with pytest.raises(DomainError):Assistant(settings,service,provider).message('both','s')
    service.execute.assert_not_called()

def test_confirmation_disable_setting(settings):
    settings.ai_require_confirmation=False
    service=Mock();service.execute.return_value={'order_id':1,'quantity':1,'status':3,'approved':True,'cost':{'minor_units':0,'currency':'INR','formatted':'INR 0.00'}}
    result=Assistant(settings,service).message('Cancel order 1','s')
    assert not result['confirmations'] and len(result['tool_results'])==1

def test_settings_errors_do_not_print_secret():
    key='private-value-that-must-not-appear'
    with pytest.raises(ValidationError) as exc:
        Settings(_env_file=None,llm_provider='gemini',gemini_api_key=key,gemini_model='')
    assert key not in str(exc.value)
