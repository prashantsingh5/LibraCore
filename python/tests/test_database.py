from concurrent.futures import ThreadPoolExecutor
from threading import Barrier
from uuid import uuid4
import os
import subprocess
import pytest
import psycopg
from fastapi.testclient import TestClient
from libracore_api.config import Settings,ROOT
from libracore_api.services import CoreService,DomainError
from libracore_api.main import create_app
from libracore_api.database import migrate
pytestmark=pytest.mark.database

def buy(db,resource='B001',quantity=1,**kwargs):
    return db.execute('submit_purchase',{'resource_id':resource,'quantity':quantity,'request_id':str(uuid4()),**kwargs})

def snapshot(settings):
    with psycopg.connect(settings.db_url(True)) as c:
        return {table:c.execute('SELECT row_to_json(t) FROM '+table+' t').fetchall()
            for table in ['resources','budgets','budget_usage','budget_title_usage','loans','purchase_records','audit_events','acquisition_requests','app_state']}

def test_migrations_idempotent(db,dbsettings):
    migrate(dbsettings.db_url(True));migrate(dbsettings.db_url(True))
    with psycopg.connect(dbsettings.db_url(True)) as c:
        assert c.execute('SELECT count(*) FROM schema_migrations').fetchone()[0]==2
        assert c.execute("SELECT count(*) FROM pg_indexes WHERE schemaname='public'").fetchone()[0]>20

@pytest.mark.parametrize('rid,kind',[('B001','Book'),('J001','Journal'),('E001','EBook'),('A001','AudioBook'),('T001','Thesis'),('D001','ElectronicResource')])
def test_reconstruction(db,dbsettings,rid,kind):
    # New binding instance/connection proves persisted reconstruction, not a Python cache.
    fresh=CoreService(dbsettings.db_url(True));r=fresh.execute('get_resource',{'resource_id':rid})
    assert r['resource_type']==kind and r['holdings']==5
    if kind in ('Book','EBook'):assert r['authors']==['Scott Meyers']
    if kind=='AudioBook':assert r['duration_minutes']==90
    if kind=='Thesis':assert r['university']=='Example University' and r['category']==0
    assert fresh.execute('quote_purchase',{'resource_id':rid,'quantity':1})['cost']['minor_units']>0

def test_crud_and_literal_search(db):
    r={'resource_id':'Q','resource_type':'Book','title':'100% C++_book','currency':'INR','unit_price_minor':100,'authors':["O'Reilly"]}
    db.execute('create_resource',r)
    found=db.execute('search_resources',{'title':'%','author':"O'Reilly"})['items']
    assert [x['resource_id'] for x in found]==['Q']
    assert not db.execute('search_resources',{'title':"' OR 1=1 --"})['items']
    r['title']='Updated';db.execute('update_resource',r)
    assert db.execute('get_resource',{'resource_id':'Q'})['title']=='Updated'
    db.execute('delete_resource',{'resource_id':'Q'})
    with pytest.raises(DomainError):db.execute('get_resource',{'resource_id':'Q'})

def test_filters_sort_pagination_fts(db):
    q={'author':'meyers','title':'effective','category':0,'identifier':'978-demo-B001','from_year':2019,'to_year':2021}
    assert [r['resource_id'] for r in db.execute('search_resources',q)['items']]==['B001']
    assert db.execute('search_resources',{'q':'Scott','sort':'rank'})['items']
    page1=db.execute('search_resources',{'limit':2})['items'];page2=db.execute('search_resources',{'limit':2,'offset':2})['items']
    assert set(r['resource_id'] for r in page1).isdisjoint(r['resource_id'] for r in page2)
    db.execute('create_resource',{'resource_id':'USD','resource_type':'Book','title':'Dollar','currency':'USD','unit_price_minor':100})
    with pytest.raises(DomainError):db.execute('search_resources',{'sort':'price_asc'})

def test_vendor_modes(db):
    a=db.execute('quote_purchase',{'resource_id':'B001','quantity':1,'vendor_selection':'Cheapest'})
    b=db.execute('quote_purchase',{'resource_id':'B001','quantity':1,'vendor_selection':'Fastest'})
    assert a['cost']['minor_units']==9000 and b['cost']['minor_units']==11000
    r=buy(db,vendor_selection='HighestRated');assert r['vendor_snapshot']['vendor_id']=='V002'

def test_department_quota_and_refund(db):
    db.execute('set_quota',{'department_id':'Computer Science','category':0,'max_units':1,'max_spend':20000,'max_titles':1})
    before=db.execute('get_budget_summary',{})['spent']
    r=buy(db,department_id='Computer Science');assert r['approved']
    rejected=buy(db,department_id='Computer Science');assert not rejected['approved']
    db.execute('cancel_purchase',{'order_id':r['order_id'],'request_id':'cancel'})
    assert db.execute('get_department_budget',{'department_id':'Computer Science'})['spent']['minor_units']==0
    assert db.execute('get_budget_summary',{})['spent']==before
    original=db.execute('get_order',{'order_id':r['order_id']});assert original['status']==1 and original['current_status']==3

def test_idempotency(db):
    p={'resource_id':'B001','quantity':1,'request_id':'once'}
    first=db.execute('submit_purchase',p);second=db.execute('submit_purchase',p);assert first==second
    with pytest.raises(DomainError):db.execute('submit_purchase',{**p,'quantity':2})
    assert db.execute('get_availability',{'resource_id':'B001'})['holdings']==6

def test_atomic_batch(db):
    before=db.execute('get_availability',{'resource_id':'B001'})
    r=db.execute('batch_purchase',{'requests':[{'resource_id':'B001','quantity':1},{'resource_id':'missing','quantity':1}],'atomic':True,'request_id':'batch'})
    assert all(not x['approved'] for x in r)
    assert db.execute('get_availability',{'resource_id':'B001'})==before
    r=db.execute('batch_purchase',{'requests':[{'resource_id':'B001','quantity':1},{'resource_id':'missing','quantity':1}],'atomic':False,'request_id':'independent'})
    assert r[0]['approved'] and not r[1]['approved']

def test_partial_returns_and_identity(db):
    r=db.execute('borrow_resource',{'resource_id':'B001','borrower_id':'U001','quantity':2,'request_id':'borrow','borrowed_at':'2026-01-01T00:00:00Z','due_at':'2026-01-15T00:00:00Z'})
    for n,status in [(1,0),(1,1)]:
        out=db.execute('return_loan',{'loan_id':r['loan_id'],'quantity':n,'request_id':str(uuid4())});assert out['status']==status
    assert db.execute('get_user_loans',{'user_id':'U001'})[0]['returned_quantity']==2
    db.execute('set_user_active',{'user_id':'U002','active':False})
    with pytest.raises(DomainError):db.execute('borrow_resource',{'resource_id':'B001','borrower_id':'U002','quantity':1})

def test_rollover(db):
    before=db.execute('get_budget_summary',{})
    r=db.execute('rollover',{'fiscal_year':2027,'new_allocation':1000000,'carry_percent':10,'request_id':'roll'})
    assert r['spent']['minor_units']==0
    assert r['total']['minor_units']==1000000+before['remaining']['minor_units']//10
    with pytest.raises(DomainError):db.execute('cancel_purchase',{'order_id':1})
    new=buy(db);assert new['approved']
    assert db.execute('cancel_purchase',{'order_id':new['order_id']})['status']==3

@pytest.mark.parametrize('operation',['submit_purchase','cancel_purchase','borrow_resource','return_loan','batch_purchase'])
def test_rollback_injected_database_failure(db,dbsettings,operation):
    p={'resource_id':'B001','quantity':1,'request_id':'fail'}
    if operation=='cancel_purchase':p={'order_id':buy(db)['order_id'],'request_id':'fail'}
    if operation=='borrow_resource':p['borrower_id']='U001'
    if operation=='return_loan':
        l=db.execute('borrow_resource',{'resource_id':'B001','quantity':1,'borrower_id':'U001'})
        p={'loan_id':l['loan_id'],'quantity':1,'request_id':'fail'}
    if operation=='batch_purchase':p={'requests':[{'resource_id':'B001','quantity':1}],'atomic':True,'request_id':'fail'}
    before=snapshot(dbsettings)
    with psycopg.connect(dbsettings.db_url(True)) as c:
        c.execute("CREATE FUNCTION fail_event() RETURNS trigger LANGUAGE plpgsql AS $$ BEGIN RAISE EXCEPTION 'injected'; END $$")
        c.execute('CREATE TRIGGER fail_event BEFORE INSERT ON audit_events FOR EACH ROW EXECUTE FUNCTION fail_event()')
    with pytest.raises(DomainError):db.execute(operation,p)
    assert snapshot(dbsettings)==before

def race(dbsettings,operation,params):
    barrier=Barrier(2)
    def worker(p):
        service=CoreService(dbsettings.db_url(True));barrier.wait()
        try:
            r=service.execute(operation,p)
            return r.get('approved',True)
        except DomainError:return False
    with ThreadPoolExecutor(2) as pool:return list(pool.map(worker,params))

def test_concurrent_lending(db,dbsettings):
    # Seed has five copies; lend four, leaving exactly one.
    db.execute('borrow_resource',{'resource_id':'B001','borrower_id':'U001','quantity':4})
    results=race(dbsettings,'borrow_resource',[{'resource_id':'B001','borrower_id':u,'quantity':1,'request_id':u} for u in ['U001','U002']])
    assert sum(results)==1 and db.execute('get_availability',{'resource_id':'B001'})['available']==0

def test_concurrent_budget(db,dbsettings):
    db.execute('create_department',{'department_id':'SMALL','name':'Small'})
    db.execute('create_budget',{'budget_id':'small','department_id':'SMALL','currency':'INR','total_minor':1000,'fiscal_year':2026})
    db.execute('create_resource',{'resource_id':'CHEAP','resource_type':'Book','title':'Cheap','unit_price_minor':700,'currency':'INR'})
    results=race(dbsettings,'submit_purchase',[{'resource_id':'CHEAP','quantity':1,'department_id':'SMALL','request_id':str(i)} for i in range(2)])
    assert sum(results)==1
    assert db.execute('get_department_budget',{'department_id':'SMALL'})['remaining']['minor_units']==300

def test_concurrent_cancellation(db,dbsettings):
    order=buy(db)['order_id']
    results=race(dbsettings,'cancel_purchase',[{'order_id':order,'request_id':str(i)} for i in range(2)])
    assert sum(results)==1

def test_full_api_workflow(db,settings):
    c=TestClient(create_app(settings,db))
    assert c.get('/health').status_code==200
    assert c.get('/api/v1/resources?author=Scott').json()['items']
    quote=c.post('/api/v1/acquisitions/quote',json={'resource_id':'B001','quantity':2});assert quote.status_code==200
    order=c.post('/api/v1/acquisitions',json={'resource_id':'B001','quantity':2,'request_id':'e2e-buy'}).json();assert order['approved']
    l=c.post('/api/v1/loans',json={'resource_id':'B001','borrower_id':'U001','quantity':2,'request_id':'e2e-borrow'}).json()
    for i in range(2):
        assert c.post('/api/v1/loans/'+l['loan_id']+'/return',json={'loan_id':l['loan_id'],'quantity':1,'request_id':'e2e-return-'+str(i)}).status_code==200
    assert c.get('/api/v1/reports/lending').json()['borrowed']==0
    assert c.get('/api/v1/events').json()
    a=c.post('/api/v1/assistant/message',json={'message':'How many copies of B001 are available?','session_id':'e2e'}).json()
    assert a['tool_results'][0]['result']['available']==7
    pending=c.post('/api/v1/assistant/message',json={'message':'Cancel order '+str(order['order_id']),'session_id':'e2e'}).json()
    assert c.post('/api/v1/assistant/confirm',json={'session_id':'e2e','token':pending['confirmations'][0]['token']}).status_code==200
    assert c.get('/api/v1/resources/B001/availability').json()['holdings']==5

def test_schema_rejects_bad_state(db,dbsettings):
    with psycopg.connect(dbsettings.db_url(True)) as c:
        with pytest.raises(psycopg.errors.CheckViolation):c.execute("UPDATE resources SET lent=holdings+1 WHERE resource_id='B001'")

def test_cpp_repository_executable(db,dbsettings):
    env={**os.environ,'TEST_DATABASE_URL':dbsettings.db_url(True)}
    subprocess.run([str(ROOT/'build-v2/libracore_db_tests')],env=env,check=True)

def test_concurrent_quota(db,dbsettings):
    db.execute('set_quota',{'department_id':'Computer Science','category':0,'max_units':1,'max_spend':100000,'max_titles':1})
    results=race(dbsettings,'submit_purchase',[{'resource_id':'B001','quantity':1,'department_id':'Computer Science','request_id':'quota-'+str(i)} for i in range(2)])
    assert sum(results)==1
    summary=db.execute('get_department_budget',{'department_id':'Computer Science'})
    assert summary['categories'][0]['units_used']==1

def test_concurrent_duplicate_request(db,dbsettings):
    params={'resource_id':'B001','quantity':1,'request_id':'same-key'}
    assert all(race(dbsettings,'submit_purchase',[params,params]))
    assert db.execute('get_availability',{'resource_id':'B001'})['holdings']==6

@pytest.mark.parametrize('field,value',[('unit_price_minor',-1),('currency','US'),('resource_type','Unknown')])
def test_resource_validation_rollback(db,field,value):
    r={'resource_id':'INVALID','resource_type':'Book','title':'Invalid','currency':'INR','unit_price_minor':100}
    r[field]=value
    with pytest.raises(DomainError):db.execute('create_resource',r)
    with pytest.raises(DomainError):db.execute('get_resource',{'resource_id':'INVALID'})
