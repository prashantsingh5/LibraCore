import pytest
pytestmark=pytest.mark.binding

def core():return pytest.importorskip('_libracore',reason='C++ binding has not been built')

def test_money():
    assert core().money(-1,'usd')=={'minor_units':-1,'currency':'USD','formatted':'USD -0.01'}
    with pytest.raises(ValueError):core().money(1,'US')

@pytest.mark.parametrize('kind',['Book','Journal','ElectronicResource','EBook','AudioBook','Thesis'])
def test_resource(kind):
    r=core().validate_resource({'resource_id':'R','resource_type':kind,'title':'Title','unit_price_minor':100,'currency':'INR'})
    assert r['price']['minor_units']==100

def test_connection_failure_redacted():
    app=core().Application('host=127.0.0.1 port=1 dbname=none password=secret connect_timeout=1')
    r=app.execute('health_check',{})
    assert r['error']['code']=='DATABASE_ERROR' and 'secret' not in str(r)

@pytest.mark.parametrize('value',[1.5,True,18446744073709551615])
def test_native_integer_boundary(value):
    with pytest.raises((ValueError,OverflowError)):
        core().validate_resource({'resource_id':'R','resource_type':'Book','title':'T','unit_price_minor':value,'currency':'INR'})

@pytest.mark.parametrize('value',[1.5,True,2147483648])
def test_operation_integer_boundary_before_connection(value):
    app=core().Application('host=127.0.0.1 port=1 connect_timeout=1')
    result=app.execute('borrow_resource',{'resource_id':'B','borrower_id':'U','quantity':value})
    assert result['error']['code'] in ('INVALID_INPUT','OVERFLOW')
