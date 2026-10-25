from unittest.mock import Mock
import pytest
from fastapi.testclient import TestClient
from libracore_api.main import create_app
from libracore_api.services import DomainError

@pytest.fixture
def client(settings):
    service=Mock();service.execute.return_value={'available':5}
    return TestClient(create_app(settings,service),raise_server_exceptions=False),service

@pytest.mark.parametrize('path,operation',[
 ('/api/v1/resources','search_resources'),('/api/v1/resources/B001','get_resource'),
 ('/api/v1/resources/B001/availability','get_availability'),('/api/v1/reports/budget','get_budget_summary'),
 ('/api/v1/reports/departments/CS','get_department_budget'),('/api/v1/reports/acquisitions','get_acquisition_summary'),
 ('/api/v1/reports/lending','get_lending_summary'),('/api/v1/events','get_recent_events'),
 ('/api/v1/users/U001/loans','get_user_loans'),('/api/v1/loans/L1','get_loan'),
 ('/api/v1/acquisitions/1','get_order')])
def test_get_routes(client,path,operation):
    c,s=client;assert c.get(path).status_code==200;assert s.execute.call_args.args[0]==operation

@pytest.mark.parametrize('path,body,operation,status',[
 ('/api/v1/acquisitions/quote',{'resource_id':'B','quantity':2},'quote_purchase',200),
 ('/api/v1/acquisitions/check',{'resource_id':'B','quantity':2},'check_purchase',200),
 ('/api/v1/acquisitions',{'resource_id':'B','quantity':2,'request_id':'a'},'submit_purchase',201),
 ('/api/v1/acquisitions/1/cancel',{'order_id':1,'request_id':'a'},'cancel_purchase',200),
 ('/api/v1/loans',{'resource_id':'B','borrower_id':'U','request_id':'a'},'borrow_resource',201),
 ('/api/v1/loans/L1/return',{'loan_id':'L1','quantity':1,'request_id':'r'},'return_loan',200)])
def test_post_routes(client,path,body,operation,status):
    c,s=client;assert c.post(path,json=body).status_code==status;assert s.execute.call_args.args[0]==operation

@pytest.mark.parametrize('code,status',[('NOT_FOUND',404),('CONFLICT',409),('QUOTA_EXCEEDED',409),('INVALID_INPUT',422),('DATABASE_ERROR',503)])
def test_errors(client,code,status):
    c,s=client;s.execute.side_effect=DomainError(code,'safe')
    r=c.get('/api/v1/resources/B');assert r.status_code==status and r.json()['error']['code']==code

def test_validation(client):
    c,s=client;assert c.post('/api/v1/acquisitions',json={'resource_id':'B','quantity':-1,'request_id':'x'}).status_code==422
    assert c.get('/api/v1/resources?limit=201').status_code==422
    s.execute.assert_not_called()

def test_internal_error(client):
    c,s=client;s.execute.side_effect=RuntimeError('secret')
    r=c.get('/api/v1/resources/B');assert r.status_code==500 and 'secret' not in r.text

def test_assistant_http(client):
    c,s=client;s.execute.return_value={'order_id':1,'quantity':1,'status':3,'approved':True,'cost':{'minor_units':100,'currency':'INR','formatted':'INR 1.00'}};r=c.post('/api/v1/assistant/message',json={'message':'Cancel order 1','session_id':'s'})
    assert r.status_code==200;s.execute.assert_not_called();token=r.json()['confirmations'][0]['token']
    assert c.post('/api/v1/assistant/confirm',json={'token':token,'session_id':'s'}).status_code==200
    assert c.post('/api/v1/assistant/confirm',json={'token':token,'session_id':'s'}).status_code==409
    assert s.execute.call_count==1

def test_health_no_live(client):
    c,s=client;s.execute.return_value={'cpp_core':'ok','database':'ok'}
    assert c.get('/health').json()['ai']['provider']=='mock'
    assert s.execute.call_args.args[0]=='health_check'

def test_openapi(client):
    c,_=client;schema=c.get('/openapi.json').json()
    assert len(schema['paths'])>=20
