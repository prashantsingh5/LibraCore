from copy import deepcopy
from secrets import token_urlsafe
from threading import Lock
from time import monotonic
from ..services import DomainError

class Confirmations:
    """Short-lived, session-bound, single-use proposals. Restart discards pending proposals."""
    def __init__(self, ttl=300, capacity=1000):
        self.ttl,self.capacity=ttl,capacity
        self.pending={}
        self.lock=Lock()

    def propose(self, session, tool, parameters):
        with self.lock:
            now=monotonic()
            self.pending={k:v for k,v in self.pending.items() if v[0]>now}
            if len(self.pending)>=self.capacity:
                raise DomainError('CONFIRMATION_LIMIT','Too many pending confirmations')
            token=token_urlsafe(32)
            self.pending[token]=(now+self.ttl,session,tool,deepcopy(parameters))
        return {'requires_confirmation':True,'token':token,'action':tool,'parameters':deepcopy(parameters),'expires_in_seconds':self.ttl}

    def consume(self, token, session):
        with self.lock:
            item=self.pending.get(token)
            if not item or item[0]<=monotonic() or item[1]!=session:
                raise DomainError('INVALID_CONFIRMATION','Confirmation missing, expired, consumed or belongs to another session')
            del self.pending[token]
            return item[2],item[3]
