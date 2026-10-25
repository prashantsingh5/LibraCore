"""Deployment-only migrations. Runtime business writes are exclusively C++."""
import argparse
import hashlib
from pathlib import Path
import psycopg
from psycopg import sql
from psycopg.conninfo import conninfo_to_dict, make_conninfo
from .config import Settings, ROOT

def create_test_database(settings):
    target=conninfo_to_dict(settings.db_url(True))
    admin=dict(target,dbname='postgres')
    with psycopg.connect(make_conninfo(**admin),autocommit=True) as conn:
        name=target['dbname']
        if not conn.execute('SELECT 1 FROM pg_database WHERE datname=%s',(name,)).fetchone():
            conn.execute(sql.SQL('CREATE DATABASE {}').format(sql.Identifier(name)))

def migrate(url):
    with psycopg.connect(url) as conn:
        conn.execute('SELECT pg_advisory_xact_lock(726249301)')
        conn.execute('CREATE TABLE IF NOT EXISTS schema_migrations(version text PRIMARY KEY, checksum text NOT NULL)')
        for path in sorted((ROOT/'database/migrations').glob('*.sql')):
            content=path.read_bytes();digest=hashlib.sha256(content).hexdigest()
            old=conn.execute('SELECT checksum FROM schema_migrations WHERE version=%s',(path.name,)).fetchone()
            if old:
                if old[0]!=digest:raise RuntimeError('Applied migration checksum changed: '+path.name)
                continue
            conn.execute(content.decode())
            conn.execute('INSERT INTO schema_migrations VALUES(%s,%s)',(path.name,digest))

def reset_test(settings):
    url=settings.db_url(True)  # independently validates name and separation before connecting
    with psycopg.connect(url) as conn:
        actual=conn.execute('SELECT current_database()').fetchone()[0]
        if actual!='libracore_test':raise RuntimeError('Refusing reset outside libracore_test')
        conn.execute('DROP SCHEMA public CASCADE')
        conn.execute('CREATE SCHEMA public')
    migrate(url)

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--test',action='store_true')
    parser.add_argument('--reset-test',action='store_true')
    parser.add_argument('--create-test',action='store_true')
    args=parser.parse_args()
    settings=Settings(ai_enabled=False)
    if args.create_test:create_test_database(settings)
    if args.reset_test:reset_test(settings)
    else:migrate(settings.db_url(args.test))
    print('Database migration completed')

if __name__=='__main__':
    try:main()
    except Exception:
        raise SystemExit('Database setup failed. Check Docker health, database privileges and environment configuration; credentials were not logged.') from None
