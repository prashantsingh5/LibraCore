"""Small deterministic seed through the C++ application, never manual financial SQL."""
import argparse
from .config import Settings
from .services import CoreService, DomainError

def seed(service):
    def create(op,args):
        try:return service.execute(op,args)
        except DomainError as e:
            if e.code!='CONFLICT':raise
    create('create_department',{'department_id':'Computer Science','name':'Computer Science'})
    create('create_budget',{'budget_id':'default','fiscal_year':2026,'currency':'INR','total_minor':100000000})
    create('create_budget',{'budget_id':'cs','department_id':'Computer Science','fiscal_year':2026,'currency':'INR','total_minor':10000000})
    for uid in ['U001','U002']:
        create('create_user',{'user_id':uid,'name':uid,'email':uid.lower()+'@example.invalid','role':'Student','department_id':'Computer Science'})
    for rid,kind in [('B001','Book'),('J001','Journal'),('E001','EBook'),('A001','AudioBook'),('T001','Thesis'),('D001','ElectronicResource')]:
        resource={'resource_id':rid,'resource_type':kind,'title':'Effective C++' if rid=='B001' else 'Demo '+kind,
            'publisher':'Demo Press','publication_year':2020,'unit_price_minor':10000,'currency':'INR',
            'authors':['Scott Meyers'] if kind in ['Book','EBook'] else [],'isbn':'978-demo-'+rid if kind in ['Book','EBook'] else '',
            'issn':'demo-journal' if kind=='Journal' else '', 'university':'Example University','degree':'MSc','supervisor':'Dr Demo',
            'access_url':'https://example.invalid/resource','narrator':'Demo Narrator','duration_minutes':90}
        try:service.execute('get_resource',{'resource_id':rid})
        except DomainError as e:
            if e.code!='NOT_FOUND':raise
            service.execute('create_resource',resource)
        service.execute('submit_purchase',{'resource_id':rid,'quantity':5,'request_id':'seed-purchase-'+rid})
    for vid,name,price,rating,days in [('V001','Economy Books',9000,4.0,7),('V002','Express Books',11000,4.8,1)]:
        service.execute('set_offer',{'resource_id':'B001','vendor_id':vid,'name':name,'price_minor':price,'currency':'INR','rating':rating,'estimated_delivery_days':days})

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--test',action='store_true');args=parser.parse_args()
    try:
        s=Settings(ai_enabled=False);seed(CoreService(s.db_url(args.test)))
        print('Representative demo data ready')
    except Exception:raise SystemExit('Seed failed; verify migrations and configuration (no secrets logged).') from None
