#!/bin/bash

echo "Pgbench started..."

for i in 1 2 3 4 5;
do
    PGPASSWORD=$POSTGRES_PASSWORD pgbench -i -U $POSTGRES_USER -h $PROXY_HOST -p $PROXY_PORT $POSTGRES_DB && break || sleep 5;
    echo "Retrying ..." $i
done

echo "Result native"
PGPASSWORD=$POSTGRES_PASSWORD pgbench -c $CLIENTS -j $THREADS -P 5 -T $TIME_SEC -U $POSTGRES_USER -h $POSTGRES_HOST -p $POSTGRES_PORT $POSTGRES_DB
echo "Result with proxy"
PGPASSWORD=$POSTGRES_PASSWORD pgbench -c $CLIENTS -j $THREADS -P 5 -T $TIME_SEC -U $POSTGRES_USER -h $PROXY_HOST -p $PROXY_PORT $POSTGRES_DB

echo "Pgbench end..."
