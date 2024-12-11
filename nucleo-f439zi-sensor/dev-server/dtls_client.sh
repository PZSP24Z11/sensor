if [ $# -ne 1 ]; then
	echo "Usage: $0 <IPv6%iface>"
	exit 1
fi

SERVER_IP=$1

openssl s_client -dtls1_2 \
  -CAfile ./../../cert/ca.crt.pem \
  -connect \["$SERVER_IP"]:20220 \
  -msg
