import importlib

class DomainError(Exception):
    def __init__(self, code, message, details=None):
        super().__init__(message)
        self.code, self.message, self.details = code, message, details or {}

class CoreService:
    def __init__(self, url):
        self.app = importlib.import_module('_libracore').Application(url)

    def execute(self, operation, parameters):
        response = self.app.execute(operation, parameters)
        if not response['ok']:
            raise DomainError(**response['error'])
        return response['data']
