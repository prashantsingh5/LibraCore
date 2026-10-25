"""Opt-in PostgreSQL benchmark; resets only the isolated test database."""
import argparse
import json
from time import perf_counter
from uuid import uuid4
from .config import Settings
from .database import reset_test
from .seed import seed
from .services import CoreService

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('size',type=int,choices=[10000,100000,1000000])
    parser.add_argument('--reset-test',action='store_true',required=True)
    args=parser.parse_args()
    settings=Settings(ai_enabled=False)
    reset_test(settings)
    service=CoreService(settings.db_url(True));seed(service)
    start=perf_counter()
    for begin in range(0,args.size,1000):
        values=[{'resource_id':f'BENCH{i:07d}','resource_type':'Book','title':f'Synthetic title {i%1000}',
                 'authors':[f'Author {i%100}'],'publisher':'Benchmark Press','publication_year':1980+i%45,
                 'unit_price_minor':1000,'currency':'INR','isbn':f'ISBN-{i}'} for i in range(begin,min(begin+1000,args.size))]
        result=service.execute('import_resources',{'resources':values})
        assert result['created']==len(values)
    timings={'synthetic_resources':args.size,'seed_resources':6,'generation_ms':(perf_counter()-start)*1000}
    def measure(name,operation,parameters):
        start=perf_counter();result=service.execute(operation,parameters);timings[name]=(perf_counter()-start)*1000
        return result
    assert measure('lookup_ms','get_resource',{'resource_id':f'BENCH{args.size-1:07d}'})['resource_id']==f'BENCH{args.size-1:07d}'
    for name,params in [
        ('title_search_ms',{'title':'Synthetic title 42'}),('author_search_ms',{'author':'Author 7'}),
        ('combined_search_ms',{'title':'Synthetic','author':'Author 7','from_year':2000}),
        ('year_range_ms',{'from_year':2000,'to_year':2010}),('pagination_ms',{'offset':args.size//2,'limit':50}),
        ('full_text_ms',{'q':'Synthetic','sort':'rank'})]:
        result=measure(name,'search_resources',params);timings[name+'_rows']=len(result['items'])
    purchased=measure('purchase_ms','submit_purchase',{'resource_id':'BENCH0000000','quantity':1,'request_id':str(uuid4())})
    assert purchased['approved']
    borrowed=measure('borrow_ms','borrow_resource',{'resource_id':'BENCH0000000','borrower_id':'U001','quantity':1,'request_id':str(uuid4())})
    assert borrowed['quantity']==1
    print(json.dumps(timings,indent=2))

if __name__=='__main__':
    try:main()
    except Exception:
        raise SystemExit('Benchmark failed. Check test database connectivity and configuration; no secrets logged.') from None
