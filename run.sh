
docker ps | grep --quiet 'test_rmq'

if  [ $? -ne 0 ] ; then
    docker run -d -p 5672:5672 -p 15672:15672 --name test_rmq dockerfile/rabbitmq
    sleep 5
fi

export LD_LIBRARY_PATH=`pwd`/rabbitmq-c/build/librabbitmq

./consumer "127.0.0.1" 5672 100 &
sleep 1
./producer "127.0.0.1" 5672 100 100 &

