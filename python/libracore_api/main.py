import json
import logging
import time
from uuid import uuid4
from pydantic import ValidationError
from fastapi import FastAPI, Depends, Request
from fastapi.exceptions import RequestValidationError
from fastapi.responses import JSONResponse
from .config import Settings
from .services import CoreService, DomainError
from . import schemas as s
from .ai.assistant import Assistant

STATUS={'NOT_FOUND':404,'CONFLICT':409,'QUOTA_EXCEEDED':409,'BUDGET_EXCEEDED':409,
        'INVALID_INPUT':422,'OVERFLOW':422,'INVALID_TOOL':422,'UNKNOWN_TOOL':422,
        'UNKNOWN_OPERATION':400,'INVALID_CONFIRMATION':409,'CONFIRMATION_LIMIT':429,
        'AI_WRITE_DISABLED':403,'AI_DISABLED':503,'DATABASE_ERROR':503,'NOT_CONFIGURED':503,
        'PROVIDER_ERROR':502,'PROVIDER_QUOTA':429}

def create_app(settings=None, service=None, provider=None):
    settings=settings or Settings()
    service=service or CoreService(settings.db_url())
    assistant=Assistant(settings,service,provider)
    app=FastAPI(title='LibraCore V2',version='2.0.0')
    app.state.service=service
    app.state.assistant=assistant
    logging.basicConfig(level=settings.libracore_log_level.upper(),format='%(message)s')

    @app.middleware('http')
    async def audit(request:Request,call_next):
        rid=str(uuid4());start=time.perf_counter()
        response=await call_next(request)
        # Route template avoids logging identifiers, query strings, prompts or credentials.
        route=getattr(request.scope.get('route'),'path','unmatched')
        logging.getLogger('libracore').info(json.dumps({'request_id':rid,'method':request.method,'route':route,
            'status':response.status_code,'duration_ms':round((time.perf_counter()-start)*1000,3)}))
        response.headers['X-Request-ID']=rid
        return response

    @app.exception_handler(DomainError)
    async def domain_error(request,exc):
        return JSONResponse(status_code=STATUS.get(exc.code,500),content={'error':{'code':exc.code,'message':exc.message,'details':exc.details}})

    @app.exception_handler(ValidationError)
    @app.exception_handler(RequestValidationError)
    async def invalid(request,exc):
        return JSONResponse(status_code=422,content={'error':{'code':'INVALID_INPUT','message':'Request validation failed','details':{'fields':[list(e['loc']) for e in exc.errors()]}}})

    @app.exception_handler(Exception)
    async def internal(request,exc):
        return JSONResponse(status_code=500,content={'error':{'code':'INTERNAL_ERROR','message':'Unexpected server error','details':{}}})

    def run(op,model):
        return service.execute(op,model.model_dump(mode='json',exclude_none=True))

    @app.get('/health')
    def health():
        try:
            result=service.execute('health_check',{})
        except DomainError:
            return JSONResponse(status_code=503,content={'status':'degraded','api':'ok','cpp_core':'ok','database':'unavailable',
                'ai':{'enabled':settings.ai_enabled,'provider':settings.provider}})
        return {'status':'ok','api':'ok',**result,'ai':{'enabled':settings.ai_enabled,'provider':settings.provider,'configured':True}}

    @app.get('/api/v1/resources')
    def resources(query:s.Search=Depends()):return run('search_resources',query)
    @app.get('/api/v1/resources/{resource_id}/availability')
    def availability(resource_id:str):return run('get_availability',s.ResourceId(resource_id=resource_id))
    @app.get('/api/v1/resources/{resource_id}')
    def get_resource(resource_id:str):return run('get_resource',s.ResourceId(resource_id=resource_id))
    @app.post('/api/v1/resources',status_code=201)
    def create_resource(body:s.Resource):return run('create_resource',body)
    @app.put('/api/v1/resources/{resource_id}')
    def update_resource(resource_id:str,body:s.Resource):
        if body.resource_id!=resource_id:raise DomainError('INVALID_INPUT','Resource ID mismatch')
        return run('update_resource',body)
    @app.delete('/api/v1/resources/{resource_id}')
    def delete_resource(resource_id:str):return run('delete_resource',s.ResourceId(resource_id=resource_id))
    @app.post('/api/v1/acquisitions/quote')
    def quote(body:s.Purchase):return run('quote_purchase',body)
    @app.post('/api/v1/acquisitions/check')
    def check(body:s.Purchase):return run('check_purchase',body)
    @app.post('/api/v1/acquisitions',status_code=201)
    def purchase(body:s.Submit):return run('submit_purchase',body)
    @app.post('/api/v1/acquisitions/batch')
    def batch(body:s.Batch):return run('batch_purchase',body)
    @app.post('/api/v1/acquisitions/{order_id}/cancel')
    def cancel(order_id:int,body:s.Cancel):
        if body.order_id!=order_id:raise DomainError('INVALID_INPUT','Order ID mismatch')
        return run('cancel_purchase',body)
    @app.get('/api/v1/acquisitions/{order_id}')
    def order(order_id:int):return run('get_order',s.OrderId(order_id=order_id))
    @app.post('/api/v1/loans',status_code=201)
    def borrow(body:s.Borrow):return run('borrow_resource',body)
    @app.post('/api/v1/loans/{loan_id}/return')
    def return_loan(loan_id:str,body:s.Return):
        if body.loan_id!=loan_id:raise DomainError('INVALID_INPUT','Loan ID mismatch')
        return run('return_loan',body)
    @app.get('/api/v1/loans/{loan_id}')
    def get_loan(loan_id:str):return run('get_loan',s.LoanId(loan_id=loan_id))
    @app.get('/api/v1/users/{user_id}/loans')
    def user_loans(user_id:str,page:s.Page=Depends()):return service.execute('get_user_loans',{'user_id':user_id,**page.model_dump()})
    @app.post('/api/v1/users',status_code=201)
    def user(body:s.User):return run('create_user',body)
    @app.post('/api/v1/departments',status_code=201)
    def department(body:s.DepartmentCreate):return run('create_department',body)
    @app.post('/api/v1/budgets',status_code=201)
    def budget(body:s.BudgetCreate):return run('create_budget',body)
    @app.put('/api/v1/budgets/quota')
    def quota(body:s.Quota):return run('set_quota',body)
    @app.post('/api/v1/budgets/rollover')
    def rollover(body:s.Rollover):return run('rollover',body)
    @app.put('/api/v1/vendor-offers')
    def offer(body:s.Offer):return run('set_offer',body)
    @app.get('/api/v1/reports/budget')
    def budget_report():return service.execute('get_budget_summary',{})
    @app.get('/api/v1/reports/departments/{department_id}')
    def department_report(department_id:str):return run('get_department_budget',s.Department(department_id=department_id))
    @app.get('/api/v1/reports/acquisitions')
    def acquisition_report():return service.execute('get_acquisition_summary',{})
    @app.get('/api/v1/reports/lending')
    def lending_report():return service.execute('get_lending_summary',{})
    @app.get('/api/v1/events')
    def events(page:s.Page=Depends()):return run('get_recent_events',page)
    @app.post('/api/v1/assistant/message')
    def message(body:s.AssistantMessage):return assistant.message(body.message,body.session_id)
    @app.post('/api/v1/assistant/confirm')
    def confirm(body:s.Confirm):return assistant.confirm(body.token,body.session_id)
    return app
