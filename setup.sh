
if [ ! -d "otp_src_17.4" ]; then
    wget http://www.erlang.org/download/otp_src_17.4.tar.gz
    tar xzvf otp_src_17.4.tar.gz
    cd  otp_src_17.4
    ./configure && make -j4
fi

if [ ! -d "rabbitmq-c" ]; then
    git clone https://github.com/alanxz/rabbitmq-c
    cd rabbitmq-c
    mkdir build
    cd build
    cmake ..
fi

ERL_LIB=otp_src_17.4/lib/erl_interface/obj/x86_64-unknown-linux-gnu
ERL_INC=otp_src_17.4/lib/erl_interface/include

RMQ_LIB=rabbitmq-c/build/librabbitmq
RMQ_INC=rabbitmq-c/librabbitmq

DC_INC=../core

gcc -o producer producer.c utils.c erl_msgs.c -lei -lerl_interface -lrabbitmq -I${ERL_INC} -I${DC_INC} -I${RMQ_INC} -L${ERL_LIB} -L${RMQ_LIB}
gcc -o consumer consumer.c utils.c erl_msgs.c -lei -lerl_interface -lrabbitmq -I${ERL_INC} -I${DC_INC} -I${RMQ_INC} -L${ERL_LIB} -L${RMQ_LIB}
