#!/bin/bash
echo "UNIT-TEST.sh: Bash version ${BASH_VERSION}..."

# "" or "Windows_NT"
echo "OS="$OS
# "cygwin" or "linux-gnu" or ""
echo "OSTYPE="$OSTYPE
if [ "$OSTYPE" = "cygwin" ]; then
echo "CYGWIN detected."
TEXEPATH=exe
TDEXEEXT=.exe
RM="rm -f" 
else
echo "no CYGWIN detected."
TEXEPATH=.
TDEXEEXT=
RM=rm
fi
NCNEW=$TEXEPATH/nc$TDEXEEXT
CUR_STRFTIME=$TEXEPATH/cur_strftime$TDEXEEXT
JSS_ENCODE=$TEXEPATH/jss_encode$TDEXEEXT
JSS_DECODE=$TEXEPATH/jss_decode$TDEXEEXT
WS_ENCODE=$TEXEPATH/ws_encode$TDEXEEXT
WS_DECODE=$TEXEPATH/ws_decode$TDEXEEXT



# PORT=${2:-8000}  # If second param is given, this is load balancer port
PORT=8090 # for HTTPC_TEST
ARG1=${1:-00}
# ARG2=${2:-127.0.0.1}
if [ "$OSTYPE" = "cygwin" ]; then
ARG2=${2:-10.0.68.99}
else
ARG2=${2:-10.0.68.84}
fi
# ARG3=${3:-8090}
ARG3=${3:-80}
ARG4=$4
ARG5=$5
ARG6=$6

TEST_IDX=$ARG1
SERVER_PORT=$ARG3
if [ "$OSTYPE" = "cygwin" ]; then
SERVER_ADDR=$ARG2
LOCAL_SERVER_ADDR=$SERVER_ADDR
else
SERVER_ADDR=$ARG2
LOCAL_SERVER_ADDR=127.0.0.1
fi
# TEST_SERVER_ADDR=192.168.1.168
# TEST_SERVER_ADDR=127.0.0.1
# TEST_SERVER_ADDR=127.0.0.1
TEST_SERVER_ADDR=$SERVER_ADDR

SERVER_URL_BASE=http://$SERVER_ADDR:$SERVER_PORT
VGW_SERVER_URL_BASE=http://$SERVER_ADDR:9080

if false; then
PROG=$1
DB_FILE=/tmp/_$$.db
# DB_FILE=html/api_server.db
fi

cleanup() {
if false; then
  rm -rf $DB_FILE
  kill -9 $PID >/dev/null 2>&1
fi
}
KILL_ALL_TEST()
{
if false; then
 pkill $PROG
 pkill cloud_side_tdx
fi
}
INIT_TRAP_TEST()
{
#set -x
trap cleanup EXIT

if false; then
cleanup

if false; then
$PROG -f $DB_FILE &
PID=$!
fi
fi
}
####################################################################
# utility routine # 
# for keep-alive only server
# CURL_KEEPALIVE_OPT="-H Connection:keep-alive"
# for close&keep-alive only server
CURL_KEEPALIVE_OPT=
# xx_OUTPUT_URL can only be done on CURL

# http://superuser.com/questions/482787/gzip-all-files-without-deleting-them
gzipkeep() {
# gzip -k -- "$file" # gzip1.6
    if [ -f "$1" ] ; then
        gzip -c -- "$1" > "$1.gz"
    fi
}
gzipkeepCurDir() {
# gzip -kr .  # gzip1.6
#	find . -type f | \
#	while read -r x
#	do
#	  gzip -c "$x" > "$x.gz"
#	done
# ( -name '*.txt -or -name '*.html etc.) 
find . -type f -not \( -name '*.gz' -or -name '*[~#]' \) -exec sh -c 'gzip -c "{}" > "{}.gz"' \;	
while read file; do if [ ! -f "$file.gz" ]; then echo "Compressing $file"; gzip -c "$file" > "$file.gz"; else echo "Not overwriting $file.gz";  fi  done < <(find . -type f -not \( -name '*.gz' -or -name '*[~#]' \))
}
CURL_PUT_OUTPUT_URL()
{
        [ -z $1 ] && { echo "CURL_PUT_OUTPUT_URL() argument1 not specified"; return 1; }
        [ -z "$2" ] && { echo "CURL_PUT_OUTPUT_URL() argument2 not specified"; return 1; }
        
	curl $COOKIE -s -X PUT -d "$2" $1
	return $?
}
CURL_DELETE_OUTPUT_URL()
{
        [ -z $1 ] && { echo "CURL_DELETE_OUTPUT_URL() argument1 not specified"; return 1; }
        
	curl $COOKIE -s -X DELETE $1
	return $?
}

GET_OUTPUT_URL()
{
        [ -z $1 ] && { echo "GET_OUTPUT_URL() argument1 not specified"; return 1; }

	curl $COOKIE -s "$1"
	return $?
}
POST_OUTPUT_URL()
{
        [ -z $1 ] && { echo "POST_OUTPUT_URL() argument1 not specified"; return 1; }
        [ -z "$2" ] && { echo "POST_OUTPUT_URL() argument2 not specified"; return 1; }
        
	curl $COOKIE -s -X POST -d "$2" "$1"
	return $?
}
B_POST_OUTPUT_URL()
{
        [ -z $1 ] && { echo "B_POST_OUTPUT_URL() argument1 not specified"; return 1; }
        [ -z "$2" ] && { echo "B_POST_OUTPUT_URL() argument2 not specified"; return 1; }

# echo "B_POST_OUTPUT_URL()" curl $COOKIE -s -X POST --data-binary "@$2" "$1"        
	curl $COOKIE -s -X POST --data-binary "@$2" "$1"
	return $?
}

TT_POST_OUTPUT_URL()
{
        [ -z $1 ] && { echo "POST_OUTPUT_URL() argument1 not specified"; return 1; }
        [ -z "$2" ] && { echo "POST_OUTPUT_URL() argument2 not specified"; return 1; }
        
#	curl $COOKIE -H "Connection: keep-alive" -s -X POST -d "$2" "$1" -m $3
	curl $COOKIE $CURL_KEEPALIVE_OPT -s -X POST -d "$2" "$1" -m $3
	return $?
}

# xx_SAVE_URL can be done on CURLorWGET
TT_GET_SAVE_URL()
{
        [ -z $1 ] && { echo "TT_GET_SAVE_URL() argument1 not specified"; return 1; }
        [ -z $2 ] && { echo "TT_GET_SAVE_URL() argument2 not specified"; return 1; }
        [ -z $3 ] && { echo "TT_GET_SAVE_URL() argument3 not specified"; return 1; }

#	wget "$1" -O $2 --quiet
#	curl $COOKIE -H "Connection: keep-alive" -s "$1" -m $3 > $2
	curl $COOKIE $CURL_KEEPALIVE_OPT -s "$1" --output "$2" -m $3 
	return $?
}
T_GET_SAVE_URL()
{
# echo "arg1 is " "$1"
# echo "arg2 is " "$2"

        [ -z $1 ] && { echo "T_GET_SAVE_URL() argument1 not specified"; return 1; }
        [ -z $2 ] && { echo "T_GET_SAVE_URL() argument2 not specified"; return 1; }

#	wget "$1" -O $2 --quiet
#	curl $COOKIE -s "$1" > $2
	curl $COOKIE -s "$1" --output "$2" 
	return $?
}
GET_SAVE_URL()
{
        [ -z $1 ] && { echo "GET_SAVE_URL() argument1 not specified"; return 1; }
        [ -z $2 ] && { echo "GET_SAVE_URL() argument2 not specified"; return 1; }

#	wget "$1" -O $2 --quiet
#	curl $COOKIE -s "$1" > $2
	curl $COOKIE -s "$1" --output "$2"
	return $?
}
POST_SAVE_URL()
{
        [ -z $1 ] && { echo "POST_SAVE_URL() argument1 not specified"; return 1; }
        [ -z "$2" ] && { echo "POST_SAVE_URL() argument2 not specified"; return 1; }
        [ -z $3 ] && { echo "POST_SAVE_URL() argument3 not specified"; return 1; }

#	wget $1 --post-data="$2" -O $3 --quiet
	curl $COOKIE -s -X POST -d "$2" "$1" > $3
#	curl $COOKIE -s -X POST -d "$2" "$1" -o $3
	return $?
}
TT_POST_SAVE_URL()
{
        [ -z $1 ] && { echo "TT_POST_SAVE_URL() argument1 not specified"; return 1; }
        [ -z "$2" ] && { echo "TT_POST_SAVE_URL() argument2 not specified"; return 1; }
        [ -z $3 ] && { echo "TT_POST_SAVE_URL() argument3 not specified"; return 1; }
        [ -z $4 ] && { echo "TT_POST_SAVE_URL() argument4 not specified"; return 1; }

	curl $COOKIE $CURL_KEEPALIVE_OPT -s -X POST -d "$2" "$1" -m $3 --output "$4"
	return $?
}

WGET_GET_SAVE_URL()
{
        [ -z $1 ] && { echo "WGET_GET_SAVE_URL() argument1 not specified"; return 1; }
        [ -z $2 ] && { echo "WGET_GET_SAVE_URL() argument2 not specified"; return 1; }

	wget $1 -O $2 --quiet
	return $?
}
WGET_POST_SAVE_URL()
{
        [ -z $1 ] && { echo "WGET_POST_SAVE_URL() argument1 not specified"; return 1; }
        [ -z "$2" ] && { echo "WGET_POST_SAVE_URL() argument2 not specified"; return 1; }
        [ -z $3 ] && { echo "WGET_POST_SAVE_URL() argument3 not specified"; return 1; }

	wget $1 --post-data="$2" -O $3 --quiet
	return $?
}


CURL_GET_OUTPUT_URL()
{
        [ -z $1 ] && { echo "CURL_GET_OUTPUT_URL() argument1 not specified"; return 1; }

	curl $COOKIE -s $1
	return $?
}
CURL_POST_OUTPUT_URL()
{
        [ -z $1 ] && { echo "CURL_POST_OUTPUT_URL() argument1 not specified"; return 1; }
        [ -z "$2" ] && { echo "CURL_POST_OUTPUT_URL() argument2 not specified"; return 1; }
        
	curl $COOKIE -s -X POST -d "$2" $1
	return $?
}

T_POST_OUTPUT_URL()
{
        [ -z $1 ] && { echo "POST_OUTPUT_URL() argument1 not specified"; return 1; }
        [ -z "$2" ] && { echo "POST_OUTPUT_URL() argument2 not specified"; return 1; }
        
	curl $COOKIE -s -X POST -d "$2" -F "$3" -d "$4" "$1"
	return $?
}


# -q is after EOF
# -w is timeout
# EMS_VGWUDP_ADDR="$LOCAL_SERVER_ADDR 9080"
EMS_VGWUDP_ADDR="$TEST_SERVER_ADDR 9080"
NC_UDP_AFTEREOF_TIMEOUT=1
NC_TCP_AFTEREOF_TIMEOUT=1
NC_AFTEREOF_TIMEOUT=1
NC_WAIT_TIMEOUT=3
if true; then
NC_ON_CLOSE_OPT="-q $NC_AFTEREOF_TIMEOUT"
NC=nc
NCNEW_EMS_VGWUDP_ADDR="udp://$TEST_SERVER_ADDR:9080"
NCnew=$NCNEW
else
NC_ON_CLOSE_OPT="--close"
NC=./netcat
# -e only for TCP
fi
if false; then
WS_DECODE_OPT=
WS_ENCODE_OPT=
else
WS_DECODE_OPT=-S
WS_ENCODE_OPT=-S
fi

if false; then
VGW_WS_EXCHANGE0()
{
	$WS_ENCODE $WS_ENCODE_OPT | $NC -p $1 -u $EMS_VGWUDP_ADDR $NC_ON_CLOSE_OPT -w $NC_WAIT_TIMEOUT | $WS_DECODE $WS_DECODE_OPT 
	return $?
}
VGW_WS_EXCHANGEpassive()
{ 
	mkfifo cmd_pipe;mkfifo res_pipe
	ls -l *_pipe
	./plain_agent <cmd_pipe >res_pipe & 
	$WS_ENCODE $WS_ENCODE_OPT <res_pipe | $NC -p $1 -u $EMS_VGWUDP_ADDR $NC_ON_CLOSE_OPT -w $2 | $WS_DECODE $WS_DECODE_OPT >cmd_pipe 
	ls -l *_pipe *.json
	$RM cmd_pipe
	$RM res_pipe
}
fi

VGW_WS_EXCHANGEboth2() 
{
#	printf "$2" | ./ws_encode $WS_ENCODE_OPT | $NC -p $1 -u $EMS_VGWUDP_ADDR $NC_ON_CLOSE_OPT -w $3 | ./ws_decode $WS_DECODE_OPT 
	printf "$2" | $WS_ENCODE $WS_ENCODE_OPT |$NCnew -W -l -A $NCNEW_EMS_VGWUDP_ADDR $NC_ON_CLOSE_OPT udp://$1 | $WS_DECODE $WS_DECODE_OPT 
	return $?
}
VGW_WS_EXCHANGEboth() 
{
#	echo "$2" | ./ws_encode $WS_ENCODE_OPT | $NC -p $1 -u $EMS_VGWUDP_ADDR $NC_ON_CLOSE_OPT -w $NC_WAIT_TIMEOUT | ./ws_decode $WS_DECODE_OPT 
	echo "$2" | $WS_ENCODE $WS_ENCODE_OPT | $NCnew -W -l -A $NCNEW_EMS_VGWUDP_ADDR $NC_ON_CLOSE_OPT udp://$1  | $WS_DECODE $WS_DECODE_OPT 
	return $?
}
VGW_WS_EXCHANGEtx()
{
#	./ws_encode $WS_ENCODE_OPT | $NC -p $1 -u $EMS_VGWUDP_ADDR $NC_ON_CLOSE_OPT -w $2 | ./ws_decode $WS_DECODE_OPT 
	$WS_ENCODE $WS_ENCODE_OPT | $NCnew -W -l -t $NCNEW_EMS_VGWUDP_ADDR $NC_ON_CLOSE_OPT udp://$1 | $WS_DECODE $WS_DECODE_OPT 
	return $?
}
VGW_WS_EXCHANGErx()
{
#	./ws_encode $WS_ENCODE_OPT | $NC -p $1 -u $EMS_VGWUDP_ADDR $NC_ON_CLOSE_OPT -w $2 | ./ws_decode $WS_DECODE_OPT 
	$WS_ENCODE $WS_ENCODE_OPT | $NCnew -W -l -r $NC_ON_CLOSE_OPT udp://$1 | $WS_DECODE $WS_DECODE_OPT 
	return $?
}

TCP_EXCHANGE0()
{
#	$NC -w $NC_WAIT_TIMEOUT -q $NC_TCP_AFTEREOF_TIMEOUT $LOCAL_SERVER_ADDR $1
	$NCnew tcp://$LOCAL_SERVER_ADDR:$1
	return $?
}
UDP_EXCHANGE0()
{
#	$NC -w $NC_WAIT_TIMEOUT -q $NC_UDP_AFTEREOF_TIMEOUT -u $LOCAL_SERVER_ADDR $1
	$NCnew udp://$LOCAL_SERVER_ADDR:$1
	return $?
}

my_pkill()
{
	PID=$(ps | grep $1 | awk '{print $1}')
	if [ "$PID" = "" ]; then
		echo "No Proc to kill found"
	else
		echo "PID="$PID"<EOT>"
		kill $PID
	fi
}


NC_UDP_TEST()
{
	echo "NC UDP Testing..."
if [ "$OSTYPE" = "cygwin" ]; then
	echo "NC UDP SKIP..."
else
	printf "sleep 2\nhello\n" | nc -w 10 -q 1 -ul $UDP_SELF_PORT0 > "$TEST_FILE_BASE"outputNC0.txt &
	echo "WHY" | nc -w 10 -q 1 -u -p $UDP_SELF_PORT2 $LOCAL_SERVER_ADDR $UDP_SELF_PORT1  > "$TEST_FILE_BASE"outputNC2.txt  &
	echo "HELLO" | nc -w 10 -q 1 -u -p $UDP_SELF_PORT1 $LOCAL_SERVER_ADDR $UDP_SELF_PORT0  > "$TEST_FILE_BASE"outputNC1.txt 
	$RM "$TEST_FILE_BASE"outputNC?.txt

# echo 'UDP Test File' > udp-text-file.txt
# nc -w 4 -luv 6871 > scan-file.txt &
# nc -w 4 -vu $LOCAL_SERVER_ADDR 6871 < udp-text-file.txt &
# sleep 7
# cat scan-file.txt
# ps
# rm udp-text-file.txt
fi
	echo "NC UDP Test End."
}


WEBSOCK_TEST()
{
	echo "Websocket Test.(nc).."
if [ "$OSTYPE" = "cygwin" ]; then
	echo "Websocket Test.(nc) SKIP..."
else
	echo ""
		# curl -i -N -H "Connection: Upgrade" -H "Upgrade: websocket" -H "Host: echo.websocket.org" -H "Origin: http://localhost:3000" http://localhost:3000/chat
		# curl $COOKIE -i -N -H "Connection: Upgrade" -H "Upgrade: websocket" -H "Sec-WebSocket-Version: 13" -H "Sec-WebSocket-Key: /60iQR0LLwJxrux5fJ9mcA==" -H "Host: $LOCAL_SERVER_ADDR" -H "Origin: http://localhost:8080" http://localhost:8080/foo
		# server ready will return from server!!
		# curl $COOKIE -i -N -H "Connection: Upgrade" -H "Upgrade: websocket" -H "Sec-WebSocket-Version: 13" -H "Sec-WebSocket-Key: /60iQR0LLwJxrux5fJ9mcA==" -H "Host: $LOCAL_SERVER_ADDR" -H "Origin: http://localhost:8080" http://localhost:8080/ws
		# curl $COOKIE -i -N -H "Connection: Upgrade" -H "Upgrade: websocket" -H "Host: echo.websocket.org" -H "Origin: http://www.websocket.org" http://echo.websocket.org
fi
	echo "Websocket Test End."
}

###############################################################################################
DEF_API_SET()
{
	HAVELOGIN=1
	HAVEVGWDB=1
	HAVEVGWIPC=1
	HAVEVGWNEOSS=1
	HAVEVGWUDP=1
	HAVEVGWWEBSOCK=1
	HAVEWEBDAV=1
	HAVEUPLOAD=1
	HAVEVGWHTTP=1
	HAVEUSERLIST=0

	HAVEPINGIPC=0

	HAVEAPIHELLO=0
	HAVEHELLO=0
	HAVEBLAH=0
	
	HAVEXMLRPC=1
	HAVEJSONRPC=0
	HAVEDBAPI=0
	HAVEAPISUM=0
	HAVEUDP=0
	HAVETCP=0
	HAVECHAT1=0
	HAVECHAT0=0
	HAVESTREAM=0
	HAVEMJPG=0
	HAVECLIENTTEST=0
}
FIND_API_SET()
{
	  AnAPI=$1
	  if [ "$AnAPI" = "apihello" ]; then
	  	HAVEAPIHELLO=1
	  fi
	  if [ "$AnAPI" = "login" ]; then
	  	HAVELOGIN=1
	  fi
	  if [ "$AnAPI" = "userlist" ]; then
	  	HAVEUSERLIST=1
	  fi
	  if [ "$AnAPI" = "hello" ]; then
	  	HAVEHELLO=1
	  fi
	  if [ "$AnAPI" = "blah" ]; then
	  	HAVEBLAH=1
	  fi
	  if [ "$AnAPI" = "xmlrpc2" ]; then
	  	HAVEXMLRPC=1
	  fi
	  if [ "$AnAPI" = "upload" ]; then
	  	HAVEUPLOAD=1
	  fi
	  if [ "$AnAPI" = "VGWdb" ]; then
	  	HAVEVGWDB=1
	  fi
	  if [ "$AnAPI" = "VGWhttp" ]; then
	  	HAVEVGWHTTP=1
	  fi
	  if [ "$AnAPI" = "VGWipc" ]; then
	  	HAVEVGWIPC=1
	  fi
	  if [ "$AnAPI" = "VGWudp" ]; then
	  	HAVEVGWUDP=1
	  fi
	  if [ "$AnAPI" = "VGWneoss" ]; then
	  	HAVEVGWNEOSS=1
	  fi
	  if [ "$AnAPI" = "VGWwebsock" ]; then
	  	HAVEVGWWEBSOCK=1
	  fi
	  if [ "$AnAPI" = "jsonrpc" ]; then
	  	HAVEJSONRPC=1
	  fi
	  if [ "$AnAPI" = "dbapi" ]; then
	  	HAVEDBAPI=1
	  fi
	  if [ "$AnAPI" = "apisum" ]; then
	  	HAVEAPISUM=1
	  fi
	  if [ "$AnAPI" = "UDP" ]; then
	  	HAVEUDP=1
	  fi
	  if [ "$AnAPI" = "TCP" ]; then
	  	HAVETCP=1
	  fi
	  if [ "$AnAPI" = "WebDAV" ]; then
	  	HAVEWEBDAV=1
	  fi
	  if [ "$AnAPI" = "chat1" ]; then
	  	HAVECHAT1=1
	  fi
	  if [ "$AnAPI" = "chat0" ]; then
	  	HAVECHAT0=1
	  fi
	  if [ "$AnAPI" = "stream" ]; then
	  	HAVESTREAM=1
	  fi
	  if [ "$AnAPI" = "mjpg" ]; then
	  	HAVEMJPG=1
	  fi
	  if [ "$AnAPI" = "clientTest" ]; then
	  	HAVECLIENTTEST=1
	  fi
	  if [ "$AnAPI" = "VGWping" ]; then
	  	HAVEPINGIPC=1
	  fi
}
API_CHECK()
{
	echo "API checking.."
	URL=$SERVER_URL_BASE/api/hello

	rm -rf $TEST_FILE_BASE"api.json"
	
	curl -s $URL -o $TEST_FILE_BASE"api.json"
	echo "checking for serverURL" $URL 
	# cat $TEST_FILE_BASE"api.json"
	test  -e "$TEST_FILE_BASE"api.json || exit 3;
	echo "API=`cat $TEST_FILE_BASE"api.json"`""<EOT>"

	RESULT0=$(cat $TEST_FILE_BASE"api.json" | cut -d ':' -f 1)
	# echo "found0=($RESULT0)"
	NUMCOUNT=30
	NUMSUB=2
	# first is ['apihello'
	RESULTi=$(echo $RESULT0 | cut -d ',' -f 1)
	anAPI=$(echo $RESULTi  | cut -d "'" -f 2)
	# echo "found= $anAPI .."
  if [ "$anAPI" = "" -o "$anAPI" = " "  -o "$anAPI" = "  " ]; then
  	echo "skip"
  else
	  FIND_API_SET $anAPI
	for (( i=2;i<=$NUMCOUNT;i++ ))
	 do
		RESULTi=$(echo $RESULT0 | cut -d ',' -f $i)
		# echo "founds=($RESULTi)"
		if [ "$RESULTi" = "" ]; then
			break
		else
		  anAPI=$(echo $RESULTi  | cut -d "'" -f 2)
		  # echo "found=($anAPI)"
		  if [ "$anAPI" = "" -o "$anAPI" = " "  -o "$anAPI" = "  " ]; then
		  	echo "skip"
		  else
			  # echo "checking"
			  FIND_API_SET $anAPI
			  # echo "checked"
		  fi
		 fi
	 done
 fi
	$RM $TEST_FILE_BASE"api.json"
	echo "HAVEAPIHELLO=$HAVEAPIHELLO,HAVELOGIN=$HAVELOGIN,HAVEHELLO=$HAVEHELLO,HAVECLIENTTEST=$HAVECLIENTTEST,HAVEJSONRPC=$HAVEJSONRPC,..."
	echo "API checking end."
}
USERLIST_TEST()
{
	if [ $HAVEUSERLIST = 1 ]; then
		echo "USERLIST_PWDCHECK start..."

		echo "get UserList..."
		URL=$SERVER_URL_BASE/ajax/get_vgw_userlist
		RESULT=$(curl -s $URL"?nojson=1&t=1")
		echo $RESULT"<EOT>"
		echo "get UserList end."

		echo "checking User&PWD..."
		URL=$SERVER_URL_BASE/ajax/get_vgw_usercheck
		RESULT=$(curl -s $URL"?user=$USERNAME&password=$PASSWORD&nojson=0&t=1")
		echo $RESULT
		echo "checking User&PWD end."

		echo "USERLIST_PWDCHECK end.."
	fi
}
LOGIN_TEST()
{
	if [ $HAVELOGIN = 1 ]; then
		echo "login.html get..."
		COOKIE="-b $TEST_FILE_BASE""my_cookies.txt"
		SAVE_COOKIE="-c $TEST_FILE_BASE""my_cookies.txt"
		RM_COOKIE="rm -rf $TEST_FILE_BASE""my_cookies.txt"
		URL=$SERVER_URL_BASE/login.html
		curl -s $URL > $TEST_FILE_BASE"login.html"
		# echo "<SOT>"`cat $TEST_FILE_BASE"login.html"`"<EOT>"
		wc $TEST_FILE_BASE"login.html"
		echo "remove login.html"
		$RM $TEST_FILE_BASE"login.html"
	
		echo "authorize start"
		URL=$SERVER_URL_BASE/authorize
		$RM_COOKIE
		# curl $SAVE_COOKIE -s -I $URL"?user=$USERNAME&password=$PASSWORD"
		curl $SAVE_COOKIE -s $URL"?user=$USERNAME&password=$PASSWORD"
		echo ""
# response_header
# HTTP/1.1 302 Found
# Set-Cookie: session=e65eb7becf8a7d9afdba12c1071cefe7; max-age=3600; http-only
# Set-Cookie: user=USER
# Set-Cookie: random=2033059225
# Set-Cookie: original_url=/; max-age=0
# Location: /

# my_cookies.txt
# 127.0.0.1	FALSE	/	FALSE	1429165277	session	7987f44ab7ae2bfa18c7ed077401774f
# 127.0.0.1	FALSE	/	FALSE	0	user	USER
# 127.0.0.1	FALSE	/	FALSE	0	random	996967336
# 127.0.0.1	FALSE	/	FALSE	1429161678	original_url	/

		echo "login end.."
	fi
}
LOGOUT_TEST()
{
	if [ $HAVELOGIN = 1 ]; then
		echo "LOGOUT start..."
		URL="$SERVER_URL_BASE/logout"
		TT_GET_SAVE_URL "$URL"  $TEST_FILE_BASE"logout.html" 3
		cat $TEST_FILE_BASE"logout.html";echo "<EOT>"
		# wc $TEST_FILE_BASE"logout.html"
		$RM $TEST_FILE_BASE"logout.html"
		$RM_COOKIE
		echo "LOGOUT end..."
	fi
}


UPLOAD_TEST()
{
	if [ $HAVEUPLOAD = 1 ]; then
		if false; then
		echo "HTTP Upload Test (SW or anyFile)..."
		# BIG_UPLOAD(user,password,MAX_FILE_SIZE,DST_FOLDER=software,file1,file2,nextURL,nextScript)
		# -F/--form <name=content> Specify HTTP multipart POST data (H)
		# curl -F "userid=1" -F "filecomment=This is an image file" -F "image=@/home/user1/Desktop/test.jpg" localhost/uploader.php
		URL=$SERVER_URL_BASE/upload
		echo "...using cookie(to folder)"
		UP_RESULT=$(curl $COOKIE -s -F "MAX_FILE_SIZE=32000000" -F "DST_FOLDER=software" -F "file1=@aa.jpg" -F "file2=@bb.jpg" $URL)
		echo $UP_RESULT
		echo "...using auth data(to folder)"
		UP_RESULT=$(curl -s -F "user=$USERNAME" -F "password=$PASSWORD" -F "MAX_FILE_SIZE=32000000" -F "DST_FOLDER=software" -F "file1=@aa.jpg" -F "file2=@bb.jpg" $URL)
		echo $UP_RESULT
		echo "HTTP Upload Test End."
		fi
	fi
}
VGW_HTTP_FILE_TEST()
{
	echo "TICB dbload device $TARGET_DEV"
	URL=$VGW_SERVER_URL_BASE/ajax/vgw_dbload
	TARGET_DEV="$VGW_DEV3_ID" 
	curl -s "$URL?conffile=vgw128_kt.xml&folder=database&device=$TARGET_DEV&mixvendor=1&nojson=1" --output  "$TEST_FILE_BASE"output1.xml
	test  -e "$TEST_FILE_BASE"output1.xml || exit 3;
	# cat "$TEST_FILE_BASE"output1.xml;echo "<EOT>"
	wc "$TEST_FILE_BASE"output1.xml
	rm -f "$TEST_FILE_BASE"output1.xml

	echo "TICB file download $TARGET_DEV"

	echo "Normal File download"
	URL=$VGW_SERVER_URL_BASE/software/cc.jpg
	curl -s $URL  --output   "$TEST_FILE_BASE"output1.jpg
	test  -e "$TEST_FILE_BASE"output1.jpg || exit 3;
	ls -l "$TEST_FILE_BASE"output1.jpg
	gzip "$TEST_FILE_BASE"output1.jpg
	ls -l "$TEST_FILE_BASE"output1.jpg.gz
	rm -f "$TEST_FILE_BASE"output1.jpg "$TEST_FILE_BASE"output1.jpg.gz
	
	echo "Normal Compressed File download"
	URL=$VGW_SERVER_URL_BASE/software/cc.jpg.gz
	curl -s $URL  --output   "$TEST_FILE_BASE"output1.jpg.gz
	test  -e "$TEST_FILE_BASE"output1.jpg.gz || exit 3;
	ls -l "$TEST_FILE_BASE"output1.jpg.gz
	gunzip "$TEST_FILE_BASE"output1.jpg.gz
	ls -l "$TEST_FILE_BASE"output1.jpg
	rm -f "$TEST_FILE_BASE"output1.jpg "$TEST_FILE_BASE"output1.jpg.gz 
	
	echo "Normal File download using Compressed encoding"
	URL=$VGW_SERVER_URL_BASE/software/cc.jpg
	curl --header 'accept-encoding: gzip' -s $URL  --output   "$TEST_FILE_BASE"output2.jpg.gz
	test  -e "$TEST_FILE_BASE"output2.jpg.gz || exit 3;
	ls -l "$TEST_FILE_BASE"output2.jpg.gz
	gunzip "$TEST_FILE_BASE"output2.jpg.gz
	ls -l "$TEST_FILE_BASE"output2.jpg
	rm -f "$TEST_FILE_BASE"output2.jpg "$TEST_FILE_BASE"output2.jpg.gz 
}
VGW_HTTPSRV_TEST()
{
	if [ $HAVEVGWHTTP = 1 ]; then
		echo "VGW HTTP Testing......"
		VGW_HTTP_FILE_TEST
		echo "VGW HTTP Testing End."
	fi
}

DIRLIST_TEST()
{
	if [ $HAVEWEBDAV = 1 ]; then
		echo "Dir Testing..."
		curl -s $COOKIE $SERVER_URL_BASE/software/ > $TEST_FILE_BASE"swdir.html"
		wc $TEST_FILE_BASE"swdir.html"
		$RM $TEST_FILE_BASE"swdir.html"
	fi
}
HELLO_TEST()
{
	if [ $HAVEHELLO = 1 ]; then
		URL=$SERVER_URL_BASE/hello
# wget $URL -O hello.txt  --quiet
		GET_SAVE_URL $URL $TEST_FILE_BASE'hello.txt'
		echo "RESULT: " `cat $TEST_FILE_BASE'hello.txt'`
		$RM $TEST_FILE_BASE'hello.txt'
	fi
}
LPING=LPING
HTTPC_TEST()
{
	if [ $HAVECLIENTTEST = 1 ]; then
		echo "HTTP client Testing.."
# URL=$SERVER_URL_BASE/clientTest
# wget $URL
# ../http_client/http_client "$LOCAL_SERVER_ADDR:$PORT" "/ajax/get_req2Dev?src=ems&dst=ems&device=VGW4321&reqstr=$LPING&t=1"
		../http_client/http_client "$LOCAL_SERVER_ADDR:$PORT" "/clientTest"
# wget "$SERVER_URL_BASE/clientTest" -O test.html --quiet
		GET_SAVE_URL "$SERVER_URL_BASE/clientTest" $TEST_FILE_BASE'test.html'
		cat $TEST_FILE_BASE'test.html';echo ""
		$RM $TEST_FILE_BASE'test.html'
		echo "HTTP client Testing End.."
	fi
}

RESTFULL_DB_TEST()
{
	if [ $HAVEDBAPI = 1 ]; then
		echo "db API testing.."
		URL=$SERVER_URL_BASE/api/v1
		RESTDB_BASE=$URL/$TEST_FILE_BASE"foo"
		RESTDB_BASE1=$URL/$TEST_FILE_BASE"bar"
	
		RESULT=$(curl $COOKIE -s -w "%{http_code}\n" -X PUT -d 'value=123' $RESTDB_BASE)
		test "$RESULT" = "200" ||!(echo $RESULT)|| exit 2
	
		# Fetch existing key
		RESULT=$(GET_OUTPUT_URL $RESTDB_BASE)
		test "$RESULT" = "123" || !(echo $RESULT) || exit 1
	
		CURL_PUT_OUTPUT_URL $RESTDB_BASE1/baz 'value=success'
	
		POST_OUTPUT_URL $RESTDB_BASE1/baz  'item=baro&value=SuccessFul'
	
		POST_SAVE_URL "$RESTDB_BASE1/baz?item=garo" 'value=SuccessFully' "$TEST_FILE_BASE"output.result 
		cat "$TEST_FILE_BASE"output.result
		$RM "$TEST_FILE_BASE"output.result
	
		# Fetch existing key
		RESULT=$(GET_OUTPUT_URL $RESTDB_BASE1/baz)
		test "$RESULT" = "success" || !(echo $RESULT) || exit 1
	
		# Fetch existing key
		RESULT=$(GET_OUTPUT_URL $RESTDB_BASE1/baz/baro)
		# echo $RESULT
		test "$RESULT" = "SuccessFul" || !(echo $RESULT) || exit 1
	
		# Fetch existing key
		RESULT=$(GET_OUTPUT_URL $RESTDB_BASE1/baz/garo)
		echo $RESULT
		test "$RESULT" = "SuccessFully" || !(echo $RESULT) || exit 1
	
		# Delete it
		curl $COOKIE -s -X DELETE $RESTDB_BASE1/baz
	
		# Make sure it's deleted - GET must result in 404
		RESULT=$(curl $COOKIE -s -w "%{http_code}\n" $RESTDB_BASE1/baz)
		test "$RESULT" = "404" ||!(echo $RESULT)|| exit 2
	
		echo "db API testing OK."
	fi
}
RESTFULL_SRV_TEST()
{
	if [ $HAVEAPISUM = 1 ]; then
		echo "RESTfull service testing.."
		URL=$SERVER_URL_BASE/api/rpc
		# RESULT=$(curl $COOKIE -s -X POST -d 'n1=123&n2=456' $URL/sum)
		RESULT=$(POST_OUTPUT_URL $URL/sum 'n1=123&n2=456')
		test "`echo $RESULT | awk '{ print $3 }'`" = "579.000000" ||!(echo $RESULT)|| exit 3 
		echo "RESTfull service testing End."
	fi
}
JSON_RPC20_TEST()
{
	if [ $HAVEJSONRPC = 1 ]; then
		echo "JSONrpc2.0 testing.."
		echo "testing POST jsonRPC2.0(curl)"
		# http://www.jsonrpc.org/specification
		# http://jsonrpc.org/historical/json-rpc-over-http.html
		URL=$SERVER_URL_BASE/api/v2
		RESULT=$(POST_OUTPUT_URL $URL/jsonrpc '{"jsonrpc":"2.0","method":"sum","params":[3,4],"id":1}')
		test "`echo $RESULT | cut -d ',' -f 3 | cut -d ':' -f 2 | cut -d '}' -f 1`" = "7" ||!(echo $RESULT)|| exit 4
	
		RESULT=$(POST_OUTPUT_URL $URL/jsonrpc '{"jsonrpc":"2.0","method":"sum","params":{"n1":3,"n2":6},"id":2}')
		test "`echo $RESULT | cut -d ',' -f 3 | cut -d ':' -f 2 | cut -d '}' -f 1`" = "9" ||!(echo $RESULT)|| exit 4
	
		RESULT=$(POST_OUTPUT_URL $URL/jsonrpc '{jsonrpc:"2.0",method:"sum",params:{n1:3,n2:4},id:3}')
		test "`echo $RESULT | cut -d ',' -f 3 | cut -d ':' -f 2 | cut -d '}' -f 1`" = "7" ||!(echo $RESULT)|| exit 4
	
		RESULT=$(POST_OUTPUT_URL $URL/jsonrpc '{jsonrpc:"2.0",method:"sum",params:[3,4],id:4}')
		test "`echo $RESULT | cut -d ',' -f 3 | cut -d ':' -f 2 | cut -d '}' -f 1`" = "7" ||!(echo $RESULT)|| exit 4
	
		RESULT=$(POST_OUTPUT_URL $URL/jsonrpc '{"jsonrpc":"2.0","method":"sum","params":[3,4,5],"id":5}')
		test "`echo $RESULT | cut -d ',' -f 3 | cut -d ':' -f 2 | cut -d '}' -f 1`" = "12" ||!(echo $RESULT)|| exit 4
	
		RESULT=$(POST_OUTPUT_URL $URL/jsonrpc '{jsonrpc:"2.0",method:"sum",params:[3,4,5],id:6}')
		test "`echo $RESULT | cut -d ',' -f 3 | cut -d ':' -f 2`" = "12}" ||!(echo $RESULT)|| exit 4
	
		RESULT=$(POST_OUTPUT_URL $URL/jsonrpc '{"jsonrpc":"2.0","method":"subtract","params":[33,3,4],"id":7}')
		test "`echo $RESULT | cut -d ',' -f 3 | cut -d ':' -f 2 | cut -d '}' -f 1`" = "26" ||!(echo $RESULT)|| exit 4
	
		RESULT=$(POST_OUTPUT_URL $URL/jsonrpc '{jsonrpc:"2.0",method:"subtract",params:[33,3,4],id:8}')
		test "`echo $RESULT | cut -d ',' -f 3 | cut -d ':' -f 2 | cut -d '}' -f 1`" = "26" ||!(echo $RESULT)|| exit 4
	
	
		RESULT=$(POST_OUTPUT_URL $URL/jsonrpc '{"jsonrpc":"2.0","method":"subtract","params":{"minuend":34,"subtrahend":13},"id":9}')
		test "`echo $RESULT | cut -d ',' -f 3 | cut -d ':' -f 2 | cut -d '}' -f 1`" = "21" ||!(echo $RESULT)|| exit 4
		RESULT=$(POST_OUTPUT_URL $URL/jsonrpc '{jsonrpc:"2.0","method":"subtract","params":{"subtrahend":13,"minuend":34},"id":10}')
		test "`echo $RESULT | cut -d ',' -f 3 | cut -d ':' -f 2 | cut -d '}' -f 1`" = "21" ||!(echo $RESULT)|| exit 4
	
		echo "testing POST jsonRPC2.0(wget or curl)"
		# using wget(POST)
		POST_SAVE_URL $URL/jsonrpc '{"jsonrpc":"2.0","method":"sum","params":[3,5],"id":11}' "$TEST_FILE_BASE"output.json
		test "`cat $TEST_FILE_BASE'output.json' | cut -d ',' -f 3 | cut -d ':' -f 2 | cut -d '}' -f 1`" = "8" ||!(cat "$TEST_FILE_BASE"output.json) || exit 4
		$RM "$TEST_FILE_BASE"output.json
	
		POST_SAVE_URL $URL/jsonrpc '{jsonrpc:"2.0",method:"sum",params:[3,5],id:12}' "$TEST_FILE_BASE"output.json
		test "`cat $TEST_FILE_BASE'output.json' | cut -d ',' -f 3 | cut -d ':' -f 2 | cut -d '}' -f 1`" = "8" ||!(cat "$TEST_FILE_BASE"output.json) || exit 4
		$RM "$TEST_FILE_BASE"output.json
	
		echo "JsonRPC2.0 test end."
	fi
}

JSON_RPC12_TEST()
{
	if [ $HAVEJSONRPC = 1 ]; then
# jsonRPC 1.2
# Encoded Parameters
# Encoding Steps:
# 1.Base 64 param value
# 2.URL Encoded Base 64 param value
# http://<end point>?method=sum&params={"a":3,"b":4}&id=2
# http://<end point>?method=sum&params=[3,4]&id=1
# http://<end point>?method=sum&params=eyJhIjozLCJiIjo0fQ%3D%3D&id=2
# http://<end point>?method=sum&params=WzMsNF0%3D&id=1

		echo "JsonRPC1.2 test start."
		URL=$SERVER_URL_BASE/api/v2

		echo "testing GET jsonRPC1.2 URLencode&b64encode"
		# using wget(GET,b64encode&URLencode)
		T_GET_SAVE_URL "$URL/jsonrpc?method=sum&params=WzMsNF0%3D&id=5" "$TEST_FILE_BASE"output.json
		test "`cat $TEST_FILE_BASE'output.json' | cut -d ',' -f 3 | cut -d ':' -f 2 | cut -d '}' -f 1`" = "7" ||!(cat "$TEST_FILE_BASE"output.json)|| exit 4
		$RM "$TEST_FILE_BASE"output.json
	
		T_GET_SAVE_URL "$URL/jsonrpc?method=sum&params=eyJhIjozLCJiIjo0fQ%3D%3D&id=5" "$TEST_FILE_BASE"output.json
		test "`cat $TEST_FILE_BASE'output.json' | cut -d ',' -f 3 | cut -d ':' -f 2 | cut -d '}' -f 1`" = "7" ||!(cat "$TEST_FILE_BASE"output.json)|| exit 4
		$RM "$TEST_FILE_BASE"output.json
	
	
		echo "testing GET jsonRPC1.2 URLencode"
		# using wget(GET)
		T_GET_SAVE_URL "$URL/jsonrpc?method=sum&params=\[5,4\]&id=5" "$TEST_FILE_BASE"output.json
		test "`cat $TEST_FILE_BASE'output.json' | cut -d ',' -f 3 | cut -d ':' -f 2 | cut -d '}' -f 1`" = "9" ||!(cat "$TEST_FILE_BASE"output.json)|| exit 4
		$RM "$TEST_FILE_BASE"output.json
	
		# my style!!
		echo "testing myStyle GET jsonRPC"
		T_GET_SAVE_URL $URL/jsonrpc?request='\{jsonrpc:"2.0",method:"sum",params:\[3,5\],id:12\}' "$TEST_FILE_BASE"output.json
		test "`cat $TEST_FILE_BASE'output.json' | cut -d ',' -f 3 | cut -d ':' -f 2 | cut -d '}' -f 1`" = "8" ||!(cat "$TEST_FILE_BASE"output.json)|| exit 4
		$RM "$TEST_FILE_BASE"output.json
	
		# support multi-call / must "id" is the last of json var of a call
		echo "testing multi-call POST jsonRPC"
		RESULT=$(curl $COOKIE -s -X POST -d '[{"method":"sum", "params":{"n1":3, "n2":4}, "id":13},{"method": "subtract", "params": {"minuend":34, "subtrahend":13}, "id":14}]' $URL/jsonrpc)
		test "`echo $RESULT | cut -d ',' -f 3 | cut -d ':' -f 2 | cut -d '}' -f 1`" = "7" ||!(echo $RESULT)|| exit 4
		test "`echo $RESULT | cut -d ',' -f 6 | cut -d ':' -f 2 | cut -d '}' -f 1`" = "21" ||!(echo $RESULT)|| exit 4
	
		echo "testing multi-call myStyle GET jsonRPC"
		T_GET_SAVE_URL $URL/jsonrpc?request='\[\{"method":"sum","params":\{"n1":3,"n2":4\},"id":13\},\{"method":"subtract","params":\{"minuend":34,"subtrahend":13\},"id":14\}\]' "$TEST_FILE_BASE"output.json
		RESULT=$(cat "$TEST_FILE_BASE"output.json)
		test "`echo $RESULT | cut -d ',' -f 3 | cut -d ':' -f 2 | cut -d '}' -f 1`" = "7" ||!(echo $RESULT)|| exit 4
		test "`echo $RESULT | cut -d ',' -f 6 | cut -d ':' -f 2 | cut -d '}' -f 1`" = "21" ||!(echo $RESULT)|| exit 4
		$RM "$TEST_FILE_BASE"output.json
	
		echo "JsonRPC1.2 test end."
	fi
}
TCP_ECHO_TEST()
{
	if [ $HAVETCP = 1 ]; then
		echo "TCP Echo.."
		RESULT=$(echo "HELLOtcp" | TCP_EXCHANGE0 $TCP_SRV_PORT)
		test "$RESULT" = "helloTCP" ||!(echo $RESULT)|| exit 4
	fi
}
TCP_JSONRPC_TEST()
{
	if [ $HAVETCP = 1 ]; then
		echo "TCP JSON RPC"
		RESULT=$(echo '[{"method":"sum", "params":{"n1":3, "n2":4}, "id":13},{"method": "subtract", "params": {"minuend":34, "subtrahend":13}, "id":14}]' | TCP_EXCHANGE0 $TCP_SRV_PORT)
		test "`echo $RESULT | cut -d ',' -f 3 | cut -d ':' -f 2 | cut -d '}' -f 1`" = "7" ||!(echo $RESULT)|| exit 4
		test "`echo $RESULT | cut -d ',' -f 6 | cut -d ':' -f 2 | cut -d '}' -f 1`" = "21" ||!(echo $RESULT)|| exit 4
	fi
}
UDP_ECHO_TEST()
{
	if [ $HAVEUDP = 1 ]; then
		echo "UDP Echo.."
		RESULT=$(echo "HELLOudp" | UDP_EXCHANGE0 $UDP_SRV_PORT)
		test "$RESULT" = "helloUDP" ||!(echo "<SOT>"$RESULT"<EOT>")|| exit 4
	fi
}
UDP_JSONRPC_TEST()
{
	if [ $HAVEUDP = 1 ]; then
		echo "UDP JSON RPC"
		RESULT=$(echo '[{"method":"sum", "params":{"n1":3, "n2":4}, "id":13},{"method": "subtract", "params": {"minuend":34, "subtrahend":13}, "id":14}]\n' | UDP_EXCHANGE0 $UDP_SRV_PORT)
		test "`echo $RESULT | cut -d ',' -f 3 | cut -d ':' -f 2 | cut -d '}' -f 1`" = "7" ||!(echo $RESULT)|| exit 4
		test "`echo $RESULT | cut -d ',' -f 6 | cut -d ':' -f 2 | cut -d '}' -f 1`" = "21" ||!(echo $RESULT)|| exit 4
	fi
}

VGW_JSON_RPC_DB_TEST()
{
	if [ $HAVEJSONRPC = 1 ]; then
		URL=$SERVER_URL_BASE/api/v2
		echo "json dblist"
		RESULT0=$(POST_OUTPUT_URL $URL/jsonrpc '{"jsonrpc":"2.0","method":"dblist","params":{"conffile":"vgw128_kt.xml","nojson":"1"},"id":1}')
		echo $RESULT0 | $JSS_DECODE 34 2 > "$TEST_FILE_BASE"sample.list
		echo "LIST:"`cat "$TEST_FILE_BASE"sample.list`"<EOT>"
		$RM "$TEST_FILE_BASE"sample.list
	
		if true; then
		echo "json dbload system (long data)"
		RESULT0=$(POST_OUTPUT_URL $URL/jsonrpc '{"jsonrpc":"2.0","method":"dbload","params":{"conffile":"vgwsystem.xml","folder":"database","device":"01","nojson":"1"},"id":1}')
		echo $RESULT0 | $JSS_DECODE 34 2 > "$TEST_FILE_BASE"sampleSys.xml
		wc "$TEST_FILE_BASE"sampleSys.xml
		fi
	
	
		if false; then
		echo "json dbsave  system (long data)"
		$JSS_ENCODE '{"jsonrpc":"2.0","method":"dbsave","params":{"conffile":"vgwsystem.xml","folder":"database","device":"01","nojson":"1","filedata":"'    '"},"id":6}'   <sampleSys.xml >"$TEST_FILE_BASE"sampleRpc0.json
		RESULT0=$( B_POST_OUTPUT_URL $URL/jsonrpc "$TEST_FILE_BASE"sampleRpc0.json )
		echo $RESULT0 | $JSS_DECODE 34 2 > "$TEST_FILE_BASE"sample.res
		cat "$TEST_FILE_BASE"sample.res
		$RM "$TEST_FILE_BASE"sample.res
		$RM "$TEST_FILE_BASE"sampleRpc0.json
		fi
	
		echo "json dbload device"
		RESULT0=$(POST_OUTPUT_URL $URL/jsonrpc '{"jsonrpc":"2.0","method":"dbload","params":{"conffile":"vgw128_kt.xml","folder":"database","device":"MG4_ID74","mixvendor":"1","nojson":"1"},"id":2}')
		# result string have backslash inserted before doubleQuote
		# clear first 34 char & last 2 char
		echo $RESULT0 | $JSS_DECODE 34 2 > "$TEST_FILE_BASE"sample1.xml
		wc "$TEST_FILE_BASE"sample1.xml
		$RM "$TEST_FILE_BASE"sample1.xml
	
		if true; then
		echo "json logload system(EMS)"
		RESULT0=$(TT_POST_OUTPUT_URL $URL/jsonrpc '{"jsonrpc":"2.0","method":"logload","params":{"conffile":"vgwsystem_log.xml","folder":"database","device":"01","st":"1428686721000","et":"1429875393000","uri":"/ajax/vgw_logload","nojson":"1"},"id":2}' $LOGLOAD_TIMEOUT)
		echo $RESULT0 | $JSS_DECODE 34 2 > "$TEST_FILE_BASE"sample.log
		wc "$TEST_FILE_BASE"sample.log
		$RM "$TEST_FILE_BASE"sample.log
		fi
	
		if false; then
		echo "json logload device"
		RESULT0=$(TT_POST_OUTPUT_URL $URL/jsonrpc '{"jsonrpc":"2.0","method":"logload","params":{"conffile":"vgw128_kt_log.xml","folder":"log","device":"01","st":"1429854432000","et":"1429866432000","uri":"/ajax/vgw_logload","nojson":"1"},"id":3}' $LOGLOAD_TIMEOUT)
		echo $RESULT0 | $JSS_DECODE 34 2 > "$TEST_FILE_BASE"sample.log
		cat "$TEST_FILE_BASE"sample.log
		$RM "$TEST_FILE_BASE"sample.log
	
		echo "json logload device"
		RESULT0=$(TT_POST_OUTPUT_URL $URL/jsonrpc '{"jsonrpc":"2.0","method":"logload","params":{"conffile":"vgw128_kt_log.xml","folder":"log","device":"01","st":"1429866432000","et":"1429865232000","uri":"/ajax/vgw_logload","nojson":"1"},"id":4}' $LOGLOAD_TIMEOUT)
		echo $RESULT0 | $JSS_DECODE 34 2 > "$TEST_FILE_BASE"sample.log
		cat "$TEST_FILE_BASE"sample.log
		$RM "$TEST_FILE_BASE"sample.log
		fi
	
		echo "json logload device(range1)"
		RESULT0=$(TT_POST_OUTPUT_URL $URL/jsonrpc '{"jsonrpc":"2.0","method":"logload","params":{"conffile":"vgw128_kt_log.xml","folder":"log","device":"112233445577","st":"1429867738000","et":"1449868136000","uri":"/ajax/vgw_logload","nojson":"1"},"id":5}' $LOGLOAD_TIMEOUT)
		echo $RESULT0 | $JSS_DECODE 34 2 > "$TEST_FILE_BASE"sample.log
		wc "$TEST_FILE_BASE"sample.log
		$RM "$TEST_FILE_BASE"sample.log
	
		echo "json logload device(range2)"
		RESULT0=$(TT_POST_OUTPUT_URL $URL/jsonrpc '{"jsonrpc":"2.0","method":"logload","params":{"conffile":"vgw128_kt_log.xml","folder":"log","device":"112233445577","st":"1429865232000","et":"1449868832000","uri":"/ajax/vgw_logload","nojson":"1"},"id":6}' $LOGLOAD_TIMEOUT)
		echo $RESULT0 | $JSS_DECODE 34 2 > "$TEST_FILE_BASE"sample.log
		wc "$TEST_FILE_BASE"sample.log
		$RM "$TEST_FILE_BASE"sample.log
	
	
	
		# echo "json dbsave  device (short data)"
		# RESULT0=$(POST_OUTPUT_URL $URL/jsonrpc '{"jsonrpc":"2.0","method":"dbsave","params":{"conffile":"vgw128_kt.xml","folder":"database","device":"MG4_ID72","nojson":"1","filedata":"HELLO"},"id":5}')
		# echo $RESULT0
		# RESULT0=$(POST_OUTPUT_URL $URL/jsonrpc '{"jsonrpc":"2.0","method":"dbload","params":{"conffile":"vgw128_kt.xml","folder":"database","device":"MG4_ID72","nojson":"1"},"id":5}')
		# echo $RESULT0 | $JSS_DECODE 34 2 > "$TEST_FILE_BASE"sample2.xml
	
		echo "json dbsave  device (long data)"
		$JSS_ENCODE '{"jsonrpc":"2.0","method":"dbsave","params":{"conffile":"vgw128_kt.xml","folder":"database","device":"MG4_ID73","nojson":"1","filedata":"'    '"},"id":6}'   <sample.xml >"$TEST_FILE_BASE"sampleRpc1.json
		RESULT0=$( B_POST_OUTPUT_URL $URL/jsonrpc "$TEST_FILE_BASE"sampleRpc1.json )
		echo $RESULT0 | $JSS_DECODE 34 2 > "$TEST_FILE_BASE"sample.res
		cat "$TEST_FILE_BASE"sample.res
		$RM "$TEST_FILE_BASE"sample.res
		$RM "$TEST_FILE_BASE"sampleRpc1.json
	
		echo "json dbload  device"
		POST_OUTPUT_URL $URL/jsonrpc '{"jsonrpc":"2.0","method":"dbload","params":{"conffile":"vgw128_kt.xml","folder":"database","device":"MG4_ID73","nojson":"1"},"id":6}'  | $JSS_DECODE 34 2 > "$TEST_FILE_BASE"sample3.xml 
		wc "$TEST_FILE_BASE"sample3.xml
		$RM "$TEST_FILE_BASE"sample3.xml
	fi
}

## short timeout may result in server SEG_FAULT!! depend on what terminal dbserver,webserver,unit_test running.

VGW_AJAX_DB_TEST()
{
	if [ $HAVEVGWDB = 1 ]; then
	printf "db time base is LOCALTIME\nLOGLOAD_TIMEOUT="$LOGLOAD_TIMEOUT "\n";echo ""
	echo "AJAX DB Testing..."
	URL=$SERVER_URL_BASE/ajax/vgw_dblist
	echo "AJAX dblist"
	GET_SAVE_URL "$URL?conffile=vgw128_kt.xml&nojson=1" "$TEST_FILE_BASE"output.list
	test  -e "$TEST_FILE_BASE"output.list || exit 3;
	echo "LIST:"`cat "$TEST_FILE_BASE"output.list`"<EOT>"
	$RM "$TEST_FILE_BASE"output.list

	if false; then
	URL=$SERVER_URL_BASE/ajax/vgw_dbload
	TARGET_DEV="$VGW_DEV3_ID" 
	echo "AJAX dbload device $TARGET_DEV"
	# wget "$URL?conffile=vgw128_kt.xml&folder=database&device=$TARGET_DEV&nojson=1"  -O "$TEST_FILE_BASE"output1.xml --quiet
	GET_SAVE_URL "$URL?conffile=vgw128_kt.xml&folder=database&device=$TARGET_DEV&mixvendor=1&nojson=1" "$TEST_FILE_BASE"output1.xml
	test  -e "$TEST_FILE_BASE"output1.xml || exit 3;
	# cat "$TEST_FILE_BASE"output1.xml;echo "<EOT>"
	wc "$TEST_FILE_BASE"output1.xml

	GET_SAVE_URL "$URL?conffile=vgw128_kt.xml&folder=database&device=$TARGET_DEV&mixvendor=0&nojson=1" "$TEST_FILE_BASE"output1a.xml
	test  -e "$TEST_FILE_BASE"output1a.xml || exit 3;
	# cat "$TEST_FILE_BASE"output1a.xml;echo "<EOT>"
	wc "$TEST_FILE_BASE"output1a.xml
	fi

	if true; then
	URL=$SERVER_URL_BASE/ajax/vgw_dbsave
	TARGET_DEV="$VGW_DEV3_ID"
	echo "AJAX dbsave device $TARGET_DEV"
	# wget "$URL?conffile=vgw128_kt.xml&folder=database&device=$TARGET_DEV&nojson=1" --post-data="filedata=`cat sample.xml`"  -O output2.json --quiet
	POST_SAVE_URL $URL?conffile=vgw128_kt.xml\&folder=database\&device=$TARGET_DEV\&noforward=1\&nojson=1 "filedata=`cat sample.xml`"  "$TEST_FILE_BASE"output2.json
	test  -e "$TEST_FILE_BASE"output2.json || exit 3;
	cat "$TEST_FILE_BASE"output2.json;echo "<EOT>"
	fi

	URL=$SERVER_URL_BASE/file/vgw_dbload
	TARGET_DEV="$VGW_DEV3_ID" 
	echo "FILE dbload device $TARGET_DEV"
	# wget "$URL?conffile=vgw128_kt.xml&folder=database&device=$TARGET_DEV&nojson=1"  -O "$TEST_FILE_BASE"output1.xml --quiet
	GET_SAVE_URL "$URL?conffile=vgw128_kt.xml&folder=database&device=$TARGET_DEV&nojson=1" "$TEST_FILE_BASE"output1.xml
	test  -e "$TEST_FILE_BASE"output1.xml || exit 3;
	# cat "$TEST_FILE_BASE"output1.xml;echo "<EOT>"
	wc "$TEST_FILE_BASE"output1.xml

	URL=$SERVER_URL_BASE/ajax/vgw_dbsave
	TARGET_DEV="$VGW_DEV4_ID"
	echo "AJAX dbsave device $TARGET_DEV"
	# wget "$URL?conffile=vgw128_kt.xml&folder=database&device=$TARGET_DEV&nojson=1" --post-data="filedata=`cat sample.xml`"  -O output2.json --quiet
	POST_SAVE_URL $URL?conffile=vgw128_kt.xml\&folder=database\&device=$TARGET_DEV\&noforward=1\&nojson=1 "filedata=`cat sample.xml`"  "$TEST_FILE_BASE"output2.json
	test  -e "$TEST_FILE_BASE"output2.json || exit 3;
	cat "$TEST_FILE_BASE"output2.json;echo "<EOT>"

	if true; then
	TARGET_DEV="01"
	echo "AJAX logload system $TARGET_DEV"
	URL=$SERVER_URL_BASE/ajax/vgw_logload
	TT_GET_SAVE_URL "$URL?conffile=vgwsystem_log.xml&folder=database&device="$TARGET_DEV"&nojson=1&st="$LOG_START_TIME0"&et="$LOG_START_TIME1"&utc=$utc"  "$TEST_FILE_BASE"output4.json $LOGLOAD_TIMEOUT
#must be NOT exist	
#	test  ! -e "$TEST_FILE_BASE"output4.json || exit 3;
#	wc "$TEST_FILE_BASE"output4.json
	fi

	if false; then
	TARGET_DEV="01"
	echo "AJAX logload device $TARGET_DEV"
	URL=$SERVER_URL_BASE/ajax/vgw_logload
	TT_GET_SAVE_URL "$URL?conffile=vgw128_kt_log.xml&folder=log&device="$TARGET_DEV"&nojson=1&st="$LOG_START_TIME0"&et="$LOG_START_TIME1"&utc=$utc"  "$TEST_FILE_BASE"output5.json $LOGLOAD_TIMEOUT
#�ȳ����� ���찡 �ִ�. ���� ���� Down	
#	test  -e "$TEST_FILE_BASE"output5.json || exit 3;
	cat "$TEST_FILE_BASE"output5.json;echo "<EOT>"
	wc "$TEST_FILE_BASE"output5.json

	echo "FILE logload device $TARGET_DEV"
	URL=$SERVER_URL_BASE/file/vgw_logload
	TT_GET_SAVE_URL "$URL?conffile=vgw128_kt_log.xml&folder=log&device="$TARGET_DEV"&nojson=1&st="$LOG_START_TIME_1"&et="$LOG_START_TIME1"&utc=$utc" "$TEST_FILE_BASE"output6.json $LOGLOAD_TIMEOUT
#	test  -e "$TEST_FILE_BASE"output6.json || exit 3;
	cat "$TEST_FILE_BASE"output6.json;echo "<EOT>"
	wc "$TEST_FILE_BASE"output6.json
	fi

	if true; then
	TARGET_DEV=$VGW_DEV2_ID
	# TARGET_DEV="112233445577"
	# TARGET_DEV="1"
	echo "AJAX logload device $TARGET_DEV"
	URL=$SERVER_URL_BASE/ajax/vgw_logload
	TT_GET_SAVE_URL "$URL?conffile=vgw128_kt_log.xml&folder=log&device="$TARGET_DEV"&nojson=1&st="$LOG_START_TIME3"&et="$LOG_START_TIME4"&utc=$utc"  "$TEST_FILE_BASE"output7.json $LOGLOAD_TIMEOUT
#	test  -e "$TEST_FILE_BASE"output7.json || exit 3;
	# cat "$TEST_FILE_BASE"output7.json;echo "<EOT>"
	wc "$TEST_FILE_BASE"output7.json

	echo "FILE logload device $TARGET_DEV"
	URL=$SERVER_URL_BASE/file/vgw_logload
	TT_GET_SAVE_URL "$URL?conffile=vgw128_kt_log.xml&folder=log&device="$TARGET_DEV"&nojson=1&st="$LOG_START_TIME3"&et="$LOG_START_TIME4"&utc=$utc" "$TEST_FILE_BASE"output8.json $LOGLOAD_TIMEOUT
#	test  -e "$TEST_FILE_BASE"output8.json || exit 3;
	# cat "$TEST_FILE_BASE"output8.json;echo "<EOT>"
	wc "$TEST_FILE_BASE"output8.json
	fi
	$RM "$TEST_FILE_BASE"output?.json
	$RM "$TEST_FILE_BASE"output?.xml
	fi
}
VGW_BLAH_TEST()
{
	if [ $HAVEBLAH = 1 ]; then
		URL=$SERVER_URL_BASE/./blah
		# wget "$URL"  -O output.txt --quiet
		GET_SAVE_URL $URL "$TEST_FILE_BASE"output.txt
		test  -e "$TEST_FILE_BASE"output.txt || exit 3;
		echo "blah_OUTPUT:" `cat "$TEST_FILE_BASE"output.txt`
		$RM "$TEST_FILE_BASE"output.txt
	fi
}

VGW_NEOSSECHO_TEST()
{
	if [ $HAVEVGWNEOSS = 1 ]; then
		URL=$SERVER_URL_BASE/ajax/post_req2Dev
		echo "AJAX request2(NEOSS 0) device(POST)"
		POST_SAVE_URL "$URL" "src=neoss&dst=neoss&device=XXXX&reqstr=NEOSS&t=1" "$TEST_FILE_BASE"output4.json
		test  -e "$TEST_FILE_BASE"output4.json || exit 3;
		cat "$TEST_FILE_BASE"output4.json;echo "<EOT>"
		$RM "$TEST_FILE_BASE"output?.json
	fi
}
VGW_IPCECHO_TEST()
{
	if [ $HAVEPINGIPC = 1 ]; then
		if true; then
			URL=$SERVER_URL_BASE/ajax/get_req2Dev
			echo "AJAX request2(PING) device(GET): URL="$URL
			# wget "$URL?src=ems&dst=ems&device=VGW4321&reqstr=$LPING&t=1" -O "$TEST_FILE_BASE"output3.xml --quiet
			GET_SAVE_URL "$URL?callback=JSONcallback_12345678&src=ems&dst=ems&device=VGW4321&reqstr=$LPING&t=1&_=212121" "$TEST_FILE_BASE"output3.json
			test  -e "$TEST_FILE_BASE"output3.json || exit 3;
			cat "$TEST_FILE_BASE"output3.json;echo "<EOT>"
		fi
		if true; then
			URL=$SERVER_URL_BASE/ajax/post_req2Dev
			echo "AJAX request2(PING) device(POST): URL="$URL
			POST_SAVE_URL "$URL" "callback=JSONcallback_22345678&src=ems&dst=ems&device=VGW4321&reqstr=$LPING&t=1&_=212121" "$TEST_FILE_BASE"output4.json
			test  -e "$TEST_FILE_BASE"output4.json || exit 3;
			cat "$TEST_FILE_BASE"output4.json;echo "<EOT>"
		fi
		rm -f "$TEST_FILE_BASE"output?.json
	fi
}

VGW_AJAX_TEST()
{
	echo "VGW AJAX Testing....."

	# sleep 10
	VGW_BLAH_TEST

	NUMTEST=5
	for (( qq=0;qq<$NUMTEST;qq++ ))
	 do
		echo "test:" $qq
		VGW_AJAX_DB_TEST
	 done
	# sleep 10

	echo "VGW AJAX Testing End."
}
VGW_READY_TEST()
{
# wait some WebSock chat session
# these can be notified by below "agentready...."&"PING"
	echo "UDP dev1 agentready to EMS(1sec for nc) by Dev1(TX/RX)"
	echo "`date`>> EMS check AJAX notify from EMS"
	#get_nodeObjLong must GET&queryString
	URL=$SERVER_URL_BASE/ajax/get_nodeObjLong
	# TT_GET_SAVE_URL "$URL?callback=JSONcall_122222222&last_id=0000&room_id=00000000&room_t=trace0001&t=1&_=12222222" "$TEST_FILE_BASE"outputR.json 2 &
	RESULT=$(VGW_WS_EXCHANGEboth $VGW_DEV1_PORT "agentready $VGW_DEV1_ID $VGW_DEV1_MAC")
	echo $RESULT
	test "$RESULT" = "OK" ||!(echo $RESULT)|| exit 4
	# sleep 2
	# cat "$TEST_FILE_BASE"outputR.json;echo "<EOT>"
	# rm -rf "$TEST_FILE_BASE"outputR.json

	echo "UDP dev2 agentready to EMS(1sec for nc) by Dev2(TX/RX)"
	echo "`date`>> EMS check AJAX notify from EMS"
	#get_nodeObjLong must GET&queryString
	URL=$SERVER_URL_BASE/ajax/get_nodeObjLong
	# TT_GET_SAVE_URL "$URL?callback=JSONcall_222222222&last_id=0000&room_id=00000000&room_t=trace0001&t=1&_=22222222" "$TEST_FILE_BASE"outputR.json 2 &
	RESULT=$(VGW_WS_EXCHANGEboth $VGW_DEV2_PORT "agentready $VGW_DEV2_ID $VGW_DEV2_MAC")
	echo $RESULT
	test "$RESULT" = "OK" ||!(echo "<SOT>"$RESULT"<EOT>")|| exit 4
	# sleep 2
	# cat "$TEST_FILE_BASE"outputR.json;echo "<EOT>"
	# rm -rf "$TEST_FILE_BASE"outputR.json
}

VGW_IPCREQ_TEST()
{
	if [ $HAVEPINGIPC = 1 ]; then
		echo "UDP PING to EMS(1sec for nc) by Dev1(TX/RX)"
		RESULT=$(VGW_WS_EXCHANGEboth $VGW_DEV1_PORT "PING hello")
		echo $RESULT
		test "$RESULT" = "PONG hello" ||!(echo $RESULT)|| exit 4
	fi

	if [ $HAVEVGWIPC = 1 ]; then
# req2Dev testing HERE
# run a NC background, this NC waiting for req & response
# send ajax req2Dev to above NC
# reponse to user!!
	echo "`date`>> EMS2Device IPC_REQ_RES starting..."
	REQ_DELAY_A=1
	REQ_DELAY_B=1
	PROCESS_DELAY=0
	RES_DELAY_A=1
	RES_DELAY_B=0
	REQ_DELAY=$(($REQ_DELAY_A+$REQ_DELAY_B))
	TOTAL_DELAY=$(($REQ_DELAY+$PROCESS_DELAY+$RES_DELAY_A+$RES_DELAY_B+2))

if true; then
	echo "`date`>> agent wait IPC_REQ from EMS(from USER)(rxOnly)"
	echo "`date`>> EMS send AJAX request to device(to agent) to Dev2"
	printf "sleep $REQ_DELAY_B\n" | VGW_WS_EXCHANGErx $VGW_DEV2_PORT $REQ_DELAY > "$TEST_FILE_BASE"sampleReq.txt &
	URL=$SERVER_URL_BASE/ajax/post_req2Dev
	TT_POST_SAVE_URL $URL "callback=JSONcall_111111111&device=$VGW_DEV2_ID&dst=agent&src=ems&reqstr=AAA_REQ abcd&t=1&_=111111111" $TOTAL_DELAY "$TEST_FILE_BASE"output4.json &
	# curl start delay
	sleep $REQ_DELAY
	echo "`date`>> check IPC_REQ"
	echo "IPC_REQ:"`cat "$TEST_FILE_BASE"sampleReq.txt`"<EOT>"
	echo "after " $PROCESS_DELAY "sec -> IPC_RES to EMS"
	sleep $PROCESS_DELAY
	IPCSRC=$(cat "$TEST_FILE_BASE"sampleReq.txt | cut -d ' ' -f 2)
	IPCDST=$(cat "$TEST_FILE_BASE"sampleReq.txt | cut -d ' ' -f 1)
	IPCREQ=$(cat "$TEST_FILE_BASE"sampleReq.txt | cut -d ' ' -f 3)
	IPCHEAD=$(cat "$TEST_FILE_BASE"sampleReq.txt | cut -d ' ' -f 3 | cut -d '_' -f 1)
	IPCPARAM=$(cat "$TEST_FILE_BASE"sampleReq.txt | cut -d ' ' -f 4)
	$RM "$TEST_FILE_BASE"sampleReq.txt

	IPCRES="$IPCSRC $IPCDST $IPCHEAD""_RES $IPCPARAM $ARG1"
	echo "IPC_RES:"$IPCRES"<EOT>"
	sleep 1
	echo "`date`>> IPC_RES from device(from agent)(txOnly)"
	printf "$IPCRES\n" | VGW_WS_EXCHANGEtx $VGW_DEV2_PORT  $RES_DELAY_A > "$TEST_FILE_BASE"sampleRes1.txt
	cat "$TEST_FILE_BASE"sampleRes1.txt;echo "<EOF>`date`"
	$RM "$TEST_FILE_BASE"sampleRes1.txt
	echo "`date`>> device after send IPC_RES"
	sleep 1
	#cat "$TEST_FILE_BASE"output4.json;echo "<EOT>`date`"
	sleep $RES_DELAY_B

	echo "`date`>> EMS check AJAX response from device(from agent)"
	RESULT=$(cat "$TEST_FILE_BASE"output4.json | cut -d ',' -f 3 | cut -d "'" -f 2 )
	cat "$TEST_FILE_BASE"output4.json;echo "<EOT>`date`"
	$RM "$TEST_FILE_BASE"output4.json
	# test "$RESULT" = "agent%20AAA_RES%20abcd%20$ARG1" ||!(echo $RESULT)|| exit 4
else
	echo "`date`>> agent wait IPC_REQ from EMS(from USER)(passive)"
	VGW_WS_EXCHANGEpassive $VGW_DEV1_PORT $TOTAL_DELAY &
	URL=$SERVER_URL_BASE/ajax/post_req2Dev
	TT_POST_SAVE_URL $URL "callback=JSONcall_111111112&device=$VGW_DEV2_ID&dst=agent&src=ems&reqstr=AAA_REQ%20abcd&t=1&_=111111112" $TOTAL_DELAY "$TEST_FILE_BASE"output4.json 
	echo "`date`>> EMS check AJAX response from device(from agent)"
	cat "$TEST_FILE_BASE"output4.json;echo "<EOT>`date`"
	$RM "$TEST_FILE_BASE"output4.json
fi	
	echo "`date`>> EMS2Device IPC_REQ_RES End..."
	
	# sleep 2

# GetLongPoll testing HERE
# longPoll waiting for Ind&Alarm from device
# send IndOrAlarm to above AjaxLongPoll
# IndOrAlarm to user!!
	IND_DELAY_A=3
	IND_DELAY_B=0
	echo "`date`>> Device2EMS IPC_IND starting..."
	IPCIND="ems agent AAA_IND ABCD $ARG1"

	echo "`date`>> EMS(user) start waiting for IPC_RES/IND..."
if false; then	
	#get_nodeObjLong must GET&queryString
	URL=$SERVER_URL_BASE/ajax/get_nodeObjLong
	TT_GET_SAVE_URL "$URL?callback=JSONcall_222232222&last_id=0000&room_id=00000000&room_t=trace0001&t=1&_=22232222" "$TEST_FILE_BASE"output5.json $IND_DELAY_A &
	# wait for bg curl start
#	sleep 1
	echo "`date`>> IPC_IND from device(from agent)(txOnly)"
	printf "sleep 1\n$IPCIND\n" | VGW_WS_EXCHANGEtx $VGW_DEV2_PORT  $RES_DELAY_A > "$TEST_FILE_BASE"sampleRes2.txt

	echo "`date`>> check IPC_IND result(on agent)"
	cat "$TEST_FILE_BASE"sampleRes2.txt;echo "<EOF>`date`"
	$RM "$TEST_FILE_BASE"sampleRes2.txt
	# wait for curl receive
	echo "`date`>> EMS check AJAX indication from device(from agent)"
	sleep 1
	sleep $IND_DELAY_A
#	sleep $IND_DELAY_B
	cat "$TEST_FILE_BASE"output5.json;echo "<EOT>`date`"
	$RM "$TEST_FILE_BASE"output5.json
	echo "`date`>> Device2EMS IPC_IND End..."
else
	echo "`date`>> IPC_IND from device(from agent)(txOnly)"
	VGW_WS_EXCHANGEboth2 $VGW_DEV2_PORT "sleep 1\n$IPCIND\n" 2 > "$TEST_FILE_BASE"sampleRes2.txt &
	echo "`date`>> EMS check AJAX indication from device(from agent)"
	#get_nodeObjLong must GET&queryString
	URL=$SERVER_URL_BASE/ajax/get_nodeObjLong
	TT_GET_SAVE_URL "$URL?callback=JSONcall_222422222&last_id=0000&room_id=00000000&room_t=trace0001&t=1&_=22224222" "$TEST_FILE_BASE"output5.json $IND_DELAY_A
	cat "$TEST_FILE_BASE"sampleRes2.txt;echo "<EOF>`date`"
	$RM "$TEST_FILE_BASE"sampleRes2.txt
	echo "`date`>> check IPC_IND result(on agent)"
	cat "$TEST_FILE_BASE"output5.json;echo "<EOT>`date`"
	RESULT=$(cat "$TEST_FILE_BASE"output5.json | cut -d ',' -f 2 | cut -d "'" -f 2 | cut -d ' ' -f 3 )
	$RM "$TEST_FILE_BASE"output5.json
	# echo $RESULT
	# can be device arrived message
	# test "$RESULT" = "AAA_IND" ||!(echo "<SOT>$RESULT<EOT>")|| exit 4

	echo "`date`>> Device2EMS IPC_IND End..."
fi	
	fi
}
VGW_UDPSRV_TEST()
{
	if [ $HAVEVGWUDP = 1 ]; then
		echo "VGW UDP IPC Testing......"
		VGW_READY_TEST
		VGW_IPCREQ_TEST
		echo "VGW UDP IPC Testing End."
	fi
}

TCP_SRV_TEST()
{
	if [ $HAVETCP = 1 ]; then
		echo "TCP server Testing......."
		TCP_ECHO_TEST
		TCP_JSONRPC_TEST
		echo "TCP server Tesing End."
	fi
}
UDP_SRV_TEST()
{
	if [ $HAVEUDP = 1 ]; then
		echo "UDP server Testing......."
		UDP_ECHO_TEST
		UDP_JSONRPC_TEST
		echo "UDP server Tesing End."
	fi
}
HTTP_JSONRPC_TEST()
{
	if [ $HAVEJSONRPC = 1 ]; then
		JSON_RPC20_TEST
		JSON_RPC12_TEST
	fi
}
RESTFULL_SERVICE_TEST()
{
	if [ $HAVEDBAPI = 1 ]; then
		RESTFULL_DB_TEST
	fi
	if [ $HAVEAPISUM = 1 ]; then
		RESTFULL_SRV_TEST
	fi
}

UPLOAD_DB_TEST()
{
	if [ $HAVEUPLOAD = 1 ]; then
		TARGET_DEV="$VGW_DEV4_ID"
		URL=$SERVER_URL_BASE/upload
		echo "HTTP Upload Test (xml or Json File)..."
		# BIG_UPLOAD(user,password,MAX_FILE_SIZE,DST_FOLDER=software,file1,file2,nextURL,nextScript)
		echo "...using cookie(to DB)"
		SCRIPT1="<script>parent.window.fileUploaded();</script>"
		SCRIPT2="<script></script>"
		UP_RESULT=$(curl $COOKIE -s -F "conffile=vgw128_kt.xml" -F "folder=database" -F "device=$TARGET_DEV" -F "nextScript= $SCRIPT1" -F "nextScript2= $SCRIPT2" -F "file1=@sample.xml" $URL)
		echo $UP_RESULT
		UP_RESULT=$(curl $COOKIE -s -F "conffile=vgw128_kt.xml" -F "folder=database" -F "device=$TARGET_DEV" -F "nextURL=/index.html" -F "file1=@sample.xml" $URL)
		# file1,file2 must be LAST iTEM!!	
		#UP_RESULT=$(curl $COOKIE -s -F "file1=@sample.xml" -F "conffile=vgw128_kt.xml" -F "folder=database" -F "device=$TARGET_DEV" -F "nextURL=/index.html" $URL)
		echo $UP_RESULT
		echo "HTTP Upload Test End."
	fi
}

HTTP_XMLRPC_TEST()
{
	if [ $HAVEXMLRPC = 1 ]; then
		echo "HTTP XML RPC"
		URL=$SERVER_URL_BASE/RPC2
		RESULT=$(TT_POST_OUTPUT_URL $URL '<?xml version="1.0"?><methodCall><methodName>system.listMethods</methodName></methodCall>' $LOGLOAD_TIMEOUT)
		printf "system.listMethods():%s\r\n" "$RESULT"
		RESULT=$(TT_POST_OUTPUT_URL $URL '<?xml version="1.0"?><methodCall><methodName>system.methodHelp</methodName><params><param><value>Hello</value></param></params></methodCall>' $LOGLOAD_TIMEOUT)
		printf "system.methodHelp('Hello'):%s\r\n" "$RESULT"
		RESULT=$(TT_POST_OUTPUT_URL $URL '<?xml version="1.0"?><methodCall><methodName>Hello</methodName></methodCall>' $LOGLOAD_TIMEOUT)
		printf "Hello():%s\r\n" "$RESULT"
		RESULT=$(TT_POST_OUTPUT_URL $URL '<?xml version="1.0"?><methodCall><methodName>HelloName</methodName><params><param><value>Chris</value></param></params></methodCall>' $LOGLOAD_TIMEOUT)
		printf "HelloName('Chris'):%s\r\n" "$RESULT"
		RESULT=$(TT_POST_OUTPUT_URL $URL '<?xml version="1.0"?><methodCall><methodName>Sum</methodName><params><param><value><double>33.330000</double></value></param><param><value><double>112.570000</double></value></param><param><value><double>76.100000</double></value></param></params></methodCall>' $LOGLOAD_TIMEOUT)
		printf "Sum(33.330000,112.570000):%s\r\n" "$RESULT"
		RESULT=$(TT_POST_OUTPUT_URL $URL '<?xml version="1.0"?><methodCall><methodName>NoSuchMethod</methodName><params><param><value><double>33.330000</double></value></param><param><value><double>112.570000</double></value></param><param><value><double>76.100000</double></value></param></params></methodCall>' $LOGLOAD_TIMEOUT)
		printf "NoSuchMethod(33.330000,112.570000):%s\r\n" "$RESULT"
		RESULT=$(TT_POST_OUTPUT_URL $URL '<?xml version="1.0"?><methodCall><methodName>system.multicall</methodName><params><param><value>'\
'<array><data>'\
'<value><struct><member><name>methodName</name><value>Sum</value></member><member><name>params</name><value><array><data><value><double>5.000000</double></value><value><double>9.000000</double></value></data></array></value></member></struct></value>'\
'<value><struct><member><name>methodName</name><value>NoSuchMethod</value></member><member><name>params</name><value><array><data><value></value></data></array></value></member></struct></value>'\
'<value><struct><member><name>methodName</name><value>Sum</value></member><member><name>params</name><value><array><data><value><double>0.000000</double></value><value><double>0.000000</double></value></data></array></value></member></struct></value>'\
'<value><struct><member><name>methodName</name><value>Sum</value></member><member><name>params</name><value><array><data><value><double>10.500000</double></value><value><double>12.500000</double></value></data></array></value></member></struct></value>'\
'<value><struct><member><name>methodName</name><value>HelloName</value></member><member><name>params</name><value><array><data><value>Chris</value></data></array></value></member></struct></value>'\
'<value><struct><member><name>methodName</name><value>system.methodHelp</value></member><member><name>params</name><value><array><data><value>Hello</value></data></array></value></member></struct></value>'\
'<value><struct><member><name>methodName</name><value>Hello</value></member><member><name>params</name><value><array><data><value></value></data></array></value></member></struct></value>'\
'<value><struct><member><name>methodName</name><value>system.listMethods</value></member><member><name>params</name><value><array><data><value></value></data></array></value></member></struct></value>'\
'</data></array>'\
'</value></param></params></methodCall>' $LOGLOAD_TIMEOUT)
		printf "system.multicall(...):%s\r\n" "$RESULT"
	fi
}
TCP_XMLRPC_TEST()
{
	if [ $HAVETCP = 1 ]; then
		echo "TCP XML RPC"
		RESULT=$(echo '<?xml version="1.0"?><methodCall><methodName>system.listMethods</methodName></methodCall>' | TCP_EXCHANGE0 $TCP_SRV_PORT)
		printf "system.listMethods():%s\r\n" "$RESULT"
		RESULT=$(echo '<?xml version="1.0"?><methodCall><methodName>system.methodHelp</methodName><params><param><value>Hello</value></param></params></methodCall>' | TCP_EXCHANGE0 $TCP_SRV_PORT)
		printf "system.methodHelp('Hello'):%s\r\n" "$RESULT"
		RESULT=$(echo '<?xml version="1.0"?><methodCall><methodName>Hello</methodName></methodCall>' | TCP_EXCHANGE0 $TCP_SRV_PORT)
		printf "Hello():%s\r\n" "$RESULT"
		RESULT=$(echo '<?xml version="1.0"?><methodCall><methodName>HelloName</methodName><params><param><value>Chris</value></param></params></methodCall>' | TCP_EXCHANGE0 $TCP_SRV_PORT)
		printf "HelloName('Chris'):%s\r\n" "$RESULT"
		RESULT=$(echo '<?xml version="1.0"?><methodCall><methodName>Sum</methodName><params><param><value><double>33.330000</double></value></param><param><value><double>112.570000</double></value></param><param><value><double>76.100000</double></value></param></params></methodCall>' | TCP_EXCHANGE0 $TCP_SRV_PORT)
		printf "Sum(33.330000,112.570000):%s\r\n" "$RESULT"
		RESULT=$(echo '<?xml version="1.0"?><methodCall><methodName>NoSuchMethod</methodName><params><param><value><double>33.330000</double></value></param><param><value><double>112.570000</double></value></param><param><value><double>76.100000</double></value></param></params></methodCall>' | TCP_EXCHANGE0 $TCP_SRV_PORT)
		printf "NoSuchMethod(33.330000,112.570000):%s\r\n" "$RESULT"
		RESULT=$(echo '<?xml version="1.0"?><methodCall><methodName>system.multicall</methodName><params><param><value>'\
'<array><data>'\
'<value><struct><member><name>methodName</name><value>Sum</value></member><member><name>params</name><value><array><data><value><double>5.000000</double></value><value><double>9.000000</double></value></data></array></value></member></struct></value>'\
'<value><struct><member><name>methodName</name><value>NoSuchMethod</value></member><member><name>params</name><value><array><data><value></value></data></array></value></member></struct></value>'\
'<value><struct><member><name>methodName</name><value>Sum</value></member><member><name>params</name><value><array><data><value><double>0.000000</double></value><value><double>0.000000</double></value></data></array></value></member></struct></value>'\
'<value><struct><member><name>methodName</name><value>Sum</value></member><member><name>params</name><value><array><data><value><double>10.500000</double></value><value><double>12.500000</double></value></data></array></value></member></struct></value>'\
'<value><struct><member><name>methodName</name><value>HelloName</value></member><member><name>params</name><value><array><data><value>Chris</value></data></array></value></member></struct></value>'\
'<value><struct><member><name>methodName</name><value>system.methodHelp</value></member><member><name>params</name><value><array><data><value>Hello</value></data></array></value></member></struct></value>'\
'<value><struct><member><name>methodName</name><value>Hello</value></member><member><name>params</name><value><array><data><value></value></data></array></value></member></struct></value>'\
'<value><struct><member><name>methodName</name><value>system.listMethods</value></member><member><name>params</name><value><array><data><value></value></data></array></value></member></struct></value>'\
'</data></array>'\
'</value></param></params></methodCall>' | TCP_EXCHANGE0 $TCP_SRV_PORT)
		printf "system.multicall(...):%s\r\n" "$RESULT"
	fi
}
UDP_XMLRPC_TEST()
{
	if [ $HAVEUDP = 1 ]; then
		echo "UDP XML RPC"
		RESULT=$(echo '<?xml version="1.0"?><methodCall><methodName>system.listMethods</methodName></methodCall>' | UDP_EXCHANGE0 $UDP_SRV_PORT)
		printf "system.listMethods():%s\r\n" "$RESULT"
		RESULT=$(echo '<?xml version="1.0"?><methodCall><methodName>system.methodHelp</methodName><params><param><value>Hello</value></param></params></methodCall>' | UDP_EXCHANGE0 $UDP_SRV_PORT)
		printf "system.methodHelp('Hello'):%s\r\n" "$RESULT"
		RESULT=$(echo '<?xml version="1.0"?><methodCall><methodName>Hello</methodName></methodCall>' | UDP_EXCHANGE0 $UDP_SRV_PORT)
		printf "Hello():%s\r\n" "$RESULT"
		RESULT=$(echo '<?xml version="1.0"?><methodCall><methodName>HelloName</methodName><params><param><value>Chris</value></param></params></methodCall>' | UDP_EXCHANGE0 $UDP_SRV_PORT)
		printf "HelloName('Chris'):%s\r\n" "$RESULT"
		RESULT=$(echo '<?xml version="1.0"?><methodCall><methodName>Sum</methodName><params><param><value><double>33.330000</double></value></param><param><value><double>112.570000</double></value></param><param><value><double>76.100000</double></value></param></params></methodCall>' | UDP_EXCHANGE0 $UDP_SRV_PORT)
		printf "Sum(33.330000,112.570000):%s\r\n" "$RESULT"
		RESULT=$(echo '<?xml version="1.0"?><methodCall><methodName>NoSuchMethod</methodName><params><param><value><double>33.330000</double></value></param><param><value><double>112.570000</double></value></param><param><value><double>76.100000</double></value></param></params></methodCall>' | UDP_EXCHANGE0 $UDP_SRV_PORT)
		printf "NoSuchMethod(33.330000,112.570000):%s\r\n" "$RESULT"
		RESULT=$(echo '<?xml version="1.0"?><methodCall><methodName>system.multicall</methodName><params><param><value>'\
'<array><data>'\
'<value><struct><member><name>methodName</name><value>Sum</value></member><member><name>params</name><value><array><data><value><double>5.000000</double></value><value><double>9.000000</double></value></data></array></value></member></struct></value>'\
'<value><struct><member><name>methodName</name><value>NoSuchMethod</value></member><member><name>params</name><value><array><data><value></value></data></array></value></member></struct></value>'\
'<value><struct><member><name>methodName</name><value>Sum</value></member><member><name>params</name><value><array><data><value><double>0.000000</double></value><value><double>0.000000</double></value></data></array></value></member></struct></value>'\
'<value><struct><member><name>methodName</name><value>Sum</value></member><member><name>params</name><value><array><data><value><double>10.500000</double></value><value><double>12.500000</double></value></data></array></value></member></struct></value>'\
'<value><struct><member><name>methodName</name><value>HelloName</value></member><member><name>params</name><value><array><data><value>Chris</value></data></array></value></member></struct></value>'\
'<value><struct><member><name>methodName</name><value>system.methodHelp</value></member><member><name>params</name><value><array><data><value>Hello</value></data></array></value></member></struct></value>'\
'<value><struct><member><name>methodName</name><value>Hello</value></member><member><name>params</name><value><array><data><value></value></data></array></value></member></struct></value>'\
'<value><struct><member><name>methodName</name><value>system.listMethods</value></member><member><name>params</name><value><array><data><value></value></data></array></value></member></struct></value>'\
'</data></array>'\
'</value></param></params></methodCall>' | UDP_EXCHANGE0 $UDP_SRV_PORT)
		printf "system.multicall(...):%s\r\n" "$RESULT"
	fi
}
XMLRPC_TEST()
{
	if [ $HAVEXMLRPC = 1 ]; then
		HTTP_XMLRPC_TEST
		TCP_XMLRPC_TEST
		UDP_XMLRPC_TEST
	fi
}
###################################################################
ALL_TEST()
{
	echo "All Test starting..."

	echo "+++" `date`
	API_CHECK
	USERLIST_TEST
	LOGIN_TEST
	UPLOAD_TEST
	DIRLIST_TEST
	HELLO_TEST
	HTTPC_TEST
	RESTFULL_SERVICE_TEST
	HTTP_JSONRPC_TEST
	echo "===" `date`
	VGW_AJAX_TEST
	VGW_JSON_RPC_DB_TEST
	XMLRPC_TEST
	echo ">>>" `date`
	WEBSOCK_TEST
	UPLOAD_DB_TEST
	echo "~~~" `date`
	VGW_IPCECHO_TEST
#	VGW_NEOSSECHO_TEST
	VGW_UDPSRV_TEST
	VGW_HTTPSRV_TEST
	echo "<<<" `date`
	LOGOUT_TEST
	echo "---" `date`
	TCP_SRV_TEST
	UDP_SRV_TEST
	if [ $HAVEUDP = 1 ]; then
		echo "___" `date`
		NC_UDP_TEST
	fi
	echo "..." `date`

	echo "All Test complete sucessfully"
}

CONFIG_TEST()
{
	USERNAME=root
	PASSWORD=tdxagw
#	PASSWORD=uml2012

	$CUR_STRFTIME %s -l
	$CUR_STRFTIME '%Y. %m. %d (%a) %H:%M:%S %Z %z' -l
	$CUR_STRFTIME '2015. 04. 24. (Fri) 15:47:12 KST' -tv
# KST
	utc=32400
# UTC
	# utc=0
	LOGLOAD_TIMEOUT=10

	LOG_START_DATETIME0="2015. 04. 24. (Fri) 14:47:12 KST"
	LOG_START_DATETIME_1="2015. 04. 24. (Fri) 17:47:12 KST"
	LOG_START_DATETIME1="2015. 04. 28. (Fri) 18:07:12 KST"
	LOG_START_DATETIME3="2015. 04. 24. (Fri) 18:27:12 KST"
	LOG_START_DATETIME4="2015. 04. 28. (Fri) 18:47:12 KST"
	LOG_START_CURRENT_DATETIME="`date`"
	LOG_START_TIME0=$($CUR_STRFTIME "$LOG_START_DATETIME0" -t)"000"
	LOG_START_TIME_1=$($CUR_STRFTIME "$LOG_START_DATETIME_1" -t)"000"
	LOG_START_TIME1=$($CUR_STRFTIME "$LOG_START_DATETIME1" -t)"000"
	LOG_START_TIME3=$($CUR_STRFTIME "$LOG_START_DATETIME3" -t)"000"
	LOG_START_TIME4=$($CUR_STRFTIME "$LOG_START_DATETIME4" -t)"000"
	LOG_START_CURRENT_TIME=$($CUR_STRFTIME "$LOG_START_CURRENT_DATETIME" -t)"000"
	echo "LOG_START_TIME0=" $LOG_START_TIME0
	echo "LOG_START_TIME_1=" $LOG_START_TIME_1
	echo "LOG_START_TIME1=" $LOG_START_TIME1
	echo "LOG_START_TIME3=" $LOG_START_TIME3
	echo "LOG_START_TIME4=" $LOG_START_TIME4
# 1428686721000 1428698721000
# 1428686721000 1428698721000
# 1428695458000 1428698721000
# 1428965414000 1428973861000
# 1428965414000 1428973861000
# LOG_START_TIME0=1428686721000
# LOG_START_TIME_1=1428965414000
# LOG_START_TIME1=1428695458000
# LOG_START_TIME3=1428965414000
# LOG_START_TIME4=1428973861000

	echo "LOG_START_TIME0:"$LOG_START_TIME0":"$LOG_START_DATETIME0
	echo "LOG_START_TIME1:"$LOG_START_TIME1":"$LOG_START_DATETIME1
	echo "LOG_START_CURRENT_TIME:"$LOG_START_CURRENT_TIME":"$LOG_START_CURRENT_DATETIME

	TCP_SRV_PORT=1234
	UDP_SRV_PORT=17006

if true; then
	TEST_FILE_BASE="output/$ARG1""__"
	UDP_SELF_BASE=22000
	VGW_AGENT_PORT_BASE0=23000
	VGW_AGENT_PORT_BASE1=24000
	VGW_AGENT_ID_BASE0=y122334455
	VGW_AGENT_ID_BASE1=1122334466
	VGW_AGENT_MAC_BASE0="00:11:22:33:44:"
	VGW_AGENT_MAC_BASE1="00:11:22:33:55:"

	VGW_DEV1_PORT=$(($VGW_AGENT_PORT_BASE0+$ARG1))
	VGW_DEV2_PORT=$(($VGW_AGENT_PORT_BASE1+$ARG1))
	VGW_DEV1_ID="$VGW_AGENT_ID_BASE0""$ARG1"
	VGW_DEV2_ID="$VGW_AGENT_ID_BASE1""$ARG1"
	VGW_DEV1_MAC="$VGW_AGENT_MAC_BASE0""$ARG1"
	VGW_DEV2_MAC="$VGW_AGENT_MAC_BASE1""$ARG1"
	VGW_DEV3_ID="VGW4321""$ARG1"
	VGW_DEV4_ID="MG4_ID74""$ARG1"
	UDP_SELF_PORT0=$(($UDP_SELF_BASE+$ARG1+0))
	UDP_SELF_PORT1=$(($UDP_SELF_BASE+$ARG1+10))
	UDP_SELF_PORT2=$(($UDP_SELF_BASE+$ARG1+20))
else
	TEST_FILE_BASE="output/00__"
	
	VGW_DEV1_PORT=32000
	VGW_DEV1_ID="112233445566"
	VGW_DEV1_MAC="00:11:22:33:44:56"
	VGW_DEV2_PORT=32001
	VGW_DEV2_ID="112233445577"
	VGW_DEV2_MAC="00:11:22:33:44:65"
	VGW_DEV3_ID ="VGW4321"
	VGW_DEV4_ID="MG4_ID74"
	UDP_SELF_PORT0=42000
	UDP_SELF_PORT1=42001
	UDP_SELF_PORT2=42002
fi

	echo "VGW_DEV1_PORT="$VGW_DEV1_PORT
	echo "VGW_DEV2_PORT="$VGW_DEV2_PORT
	echo "VGW_DEV1_ID="$VGW_DEV1_ID
	echo "VGW_DEV2_ID="$VGW_DEV2_ID
	echo "VGW_DEV1_MAC="$VGW_DEV1_MAC
	echo "VGW_DEV2_MAC="$VGW_DEV2_MAC
	echo "TEST_FILE_BASE="$TEST_FILE_BASE
	echo "TCP_SRV_PORT="$TCP_SRV_PORT
	echo "UDP_SRV_PORT="$UDP_SRV_PORT
	echo "UDP_SELF_PORT0="$UDP_SELF_PORT0
	echo "UDP_SELF_PORT1="$UDP_SELF_PORT1
	echo "UDP_SELF_PORT2="$UDP_SELF_PORT2

	DEF_API_SET
	# EMS_VGWUDP_ADDR="$LOCAL_SERVER_ADDR 9080"
	EMS_VGWUDP_ADDR="$TEST_SERVER_ADDR 9080"
}

test_cleanup() {
	rm -rf "$TEST_FILE_BASE"*
	my_pkill nc
}

main()
{
	echo "init trap"
	INIT_TRAP_TEST
	echo "config"
	CONFIG_TEST
	trap test_cleanup EXIT

	echo "all test"
	ALL_TEST
	KILL_ALL_TEST
	
	exit 0
}

###########################
# sudo ip addr add 192.168.1.240/24 dev eth5
# sudo ip addr add 192.168.1.230/24 dev eth5
main
