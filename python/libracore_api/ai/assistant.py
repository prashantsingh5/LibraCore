from dataclasses import asdict
from ..schemas import ToolOutput
from ..services import DomainError
from .providers import make_provider
from .tools import Action, declarations, validate_call, validate_output
from .confirmation import Confirmations

class Assistant:
    def __init__(self, settings, service, provider=None):
        self.settings,self.service=settings,service
        self.provider=provider or make_provider(settings)
        self.confirmations=Confirmations()

    def message(self, message, session):
        if not self.settings.ai_enabled:
            raise DomainError('AI_DISABLED','AI is disabled')
        reply=self.provider.interpret(message,declarations())
        if len(reply.calls)>1:
            raise DomainError('INVALID_TOOL','At most one operation per assistant message')
        result={'provider':asdict(reply.metadata),'tool_calls':[],'tool_results':[],'confirmations':[]}
        for call in reply.calls:
            tool,args=validate_call(call)
            result['tool_calls'].append({'name':tool.name,'arguments':args,'classification':tool.action.value})
            if tool.action != Action.READ and not self.settings.ai_write_actions_enabled:
                raise DomainError('AI_WRITE_DISABLED','AI write actions are disabled')
            if tool.action==Action.HIGH_RISK and self.settings.ai_require_confirmation:
                result['confirmations'].append(self.confirmations.propose(session,tool.name,args))
            else:
                result['tool_results'].append(ToolOutput(operation=tool.name,result=validate_output(tool.name,self.service.execute(tool.name,args))).model_dump())
        # Deterministic framing prevents ungrounded model text from claiming business outcomes.
        result['message']=('Confirmation required; no action executed.' if result['confirmations'] else
                           'Authoritative application results attached.' if result['tool_results'] else
                           'No supported action identified. Supply resource, user or order IDs, or request a search.')
        return result

    def confirm(self, token, session):
        if not self.settings.ai_enabled or not self.settings.ai_write_actions_enabled:
            raise DomainError('AI_WRITE_DISABLED','AI write actions are disabled')
        name,args=self.confirmations.consume(token,session)
        # No model invocation and no parameter replacement on this route.
        return ToolOutput(operation=name,result=validate_output(name,self.service.execute(name,args))).model_dump()
