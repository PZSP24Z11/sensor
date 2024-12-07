if [ $# -ne 1 ]; then
	echo "Usage: $0 <IPv6%iface>"
	exit 1
fi

SERVER_IP=$1

openssl s_server -dtls1_2 \
  -key ./../cert/server.key \
  -cert ./../cert/server.crt \
  -accept \["$SERVER_IP"]:20220 -msg
