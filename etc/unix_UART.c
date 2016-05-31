#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#if INCLUDE_UART2TCP_SUPPORT
//////////////////////////////////////////////////////
typedef int bool;
#define false 0
#define true (!false)
extern int g_debugLevel;

#if 0	// re-open on no data		
int KillTimer(unsigned int uiTimerId)
{
	int removed = 1;
	return removed;
}
int SetTimer(unsigned int secs,unsigned int usecs,unsigned int uiTimerId)
{
	return 0;// or -1
}
void ReadUnregisterSelfId(int sock)
{
}
int ReadRegisterSelfId(int sock,void *sock_ctx)
{
	return 0;// or -1
}

#define ms2eloopsecs(ms)		(ms)/1000,((ms)%1000)*1000
#define TIMERID_SHIFT	16
#define TIMERID_MASK	(0xffffffff<<TIMERID_SHIFT) // (~((1<<TIMERID_SHIFT)-1))	/*0xffff0000*/
#define GET_TIMERID(timerId)  (timerId&TIMERID_MASK)
#define VGWCARDMAN_SIMPLE_TID_BASE	(0x1ff<<TIMERID_SHIFT)
#define VGWCARDMAN_PIC32CON_TID	(VGWCARDMAN_SIMPLE_TID_BASE+8)
	#define VGWCARDMAN_PIC32CON_TIMEOUT1_VAL 1000
	#define VGWCARDMAN_PIC32CON_TIMEOUT2_VAL 1000

int m_sd=-1;
bool /*vgwCardMan::*/OnSerialMessage(int sock,char* strSerialMsg)
{
	return true;
}
#else
extern bool /*vgwCardMan::*/OnSerialMessage(int sock,char* strSerialMsg);
bool /*vgwCardMan::*/OnSerialMessage(int sock,char* strSerialMsg)
{
	return true;
}
#endif


/*for UART*/
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>

// extern "C"{
void safe_write(int fd,const void* pData,int len)
{
	int ret;
	
	if(errno!=0){
		perror("BEFOREwrite");
		fprintf(stderr,"fd(%d): error BEFOREwrite to fd errno%d,len=%d\n",fd,errno,len);
		errno=0;//auto clear error_no
		//sleep(3);
	}
	ret=write(fd,pData,len);
	if(ret<0){
		perror("write");
	}
	else if(ret==0){
		fprintf(stderr,"zero write to fd %d<%d\n",ret,len);
	}
	else if(ret<len){
		fprintf(stderr,"less write to fd %d<%d\n",ret,len);
	}
	if(len<=0){
		fprintf(stderr,"zero or negative write to fd %d\n",len);
	}
	if(errno!=0){
		perror("write");
		fprintf(stderr,"fd(%d): error write to fd errno%d,len=%d\n",fd,errno,len);
		errno=0;//auto clear error_no
		//sleep(3);
	}
	//if(!memcmp(pData,"\r\n",2)||memcmp(pData,"REQ-SWITCH-READ",15)){fwrite(pData,1,len,stderr);fflush(stderr);}
}
// }


// #define TTY_NAME_FMT "/dev/ttySAC%d"
#define TTY_NAME_FMT "/dev/ttyS%d"
int numErr=0;
struct  termios  oldtio;
/*
int serial_config(int fd, unsigned char baud)
{
	int ret = 0;
	struct termios oldtio,newtio;
	unsigned int baudrate = 0;

	switch(baud)
	{
		case BAUD_4800:
			baudrate = B4800;
			break;
		case BAUD_9600:
			baudrate = B9600;
			break;
		case BAUD_19200:
			baudrate = B19200;
			break;
#if 1
		case BAUD_38400:
			baudrate = B38400;
			break;
		case BAUD_57600:
			baudrate = B57600;
			break;
		case BAUD_115200:
			baudrate = B115200;
			break;
#endif
		default:
			app_err("Serial : baudrate error %x\n", baud);
			baudrate = B9600;
			ret = -EINVAL;
			break;
	}

	app_info("Serial : baudrate %d\n", get_rate(baud));

	tcgetattr( fd, &oldtio );               //save previous terminal state
 	memset( &newtio, 0, sizeof(newtio) );
 	newtio.c_cflag = baudrate | CS8 | CLOCAL | CREAD ;
 	newtio.c_iflag = IGNPAR;
 	newtio.c_oflag = OPOST | ONLCR;

 	//set input mode (non-canonical, no echo,.....)
 	newtio.c_lflag = 0;
	newtio.c_cc[VTIME] = 1; 	// 1 == 1/10 sec.
	newtio.c_cc[VMIN] = 255;// SERIAL_DATA_MAX; 
 	tcflush( fd, TCIOFLUSH );
 	tcsetattr( fd, TCSANOW, &newtio );

	return ret;
}
*/

#ifdef __APPLE__
#  include <IOKit/serial/ioss.h>
#endif
#ifdef __linux__
#  include <linux/serial.h>
#endif
#ifdef __CYGWIN__
// Cygwin doesn't provide cfmakeraw...
// this definition found in port of unix 'script' utility
// by Alan Evans (Alan_Evans AT iwv com), 2002-09-27
// http://marc.info/?l=cygwin&m=103314951904556&w=2
void cfmakeraw(struct termios *termios_p) {
  termios_p->c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP
			  |INLCR|IGNCR|ICRNL|IXON);
  termios_p->c_oflag &= ~OPOST;
  termios_p->c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
  termios_p->c_cflag &= ~(CSIZE|PARENB);
  termios_p->c_cflag |= CS8;
}
#endif
#ifndef FIONREAD    // cygwin defines FIONREAD in  socket.h  instead of  ioctl.h
#include <sys/socket.h>
#endif


#if 0
typedef int gboolean;
typedef unsigned char U1;
#define log_error printf
#define log_warn printf
#define log_info printf
#define sleep_ms(ms)	usleep(ms*100)
#define FALSE 0
#define TRUE (!FALSE)
static struct _g_context{
	gboolean uart_conflict;
}g_context;

#define SEND_RATE 115200
#define TIMEOUT (SEND_RATE / 1000 + 5)



static const char *gps_uart_file = "/dev/ttySAC0";
static int gps_dev_fd = -1;
static struct  termios  ttyset,ttyset_old;
static fd_set rs,Ws;
static int fd_count;
static struct timespec timeout;
static int flags;
/**
 * the fd is saved to local static var gps_dev_fd.
 *
 * O_NOCTTY:
 * If pathname refers to a terminal device -- see tty(4) -- it will not become
 * the process's controlling terminal even if the process does not have one.
 *
 * O_NONBLOCK or O_NDELAY:
 * When possible, the file is opened in non-blocking mode.  Neither the open()
 * nor any subsequent operations on the file descriptor which is returned  will
 * cause the calling process to wait. For the handling of FIFOs (named pipes),
 * see also fifo(7). For a discussion of the effect of O_NONBLOCK in conjunction
 * with mandatory file locks and with file leases, see fcntl(2).
 */
int uart_open(unsigned int baud_rate, gboolean verify_output)
{

	/* first open with non-blocking mode: we need
	 * 1) if GPS chip is started just now, we must wait until it gets ready.
	 * 2) else sweep possible UBX garbage */
	flags = O_RDWR | O_NOCTTY;

	if (verify_output)
		flags |= O_NONBLOCK;

	if (gps_dev_fd > 0)
		close(gps_dev_fd);

	gps_dev_fd = open(gps_uart_file, flags);

	if (gps_dev_fd < 0) {
		log_error("open device failed: %s\n", strerror(errno));
		return 0;
	}

	/* Save original terminal parameters */
	if (tcgetattr(gps_dev_fd, &ttyset_old) != 0) {
		log_error("get device attribute failed: %s", gps_uart_file);
		return 0;
	}

	speed_t speed = 0;
	switch(baud_rate) {
	case 4800:
		speed = B4800;
		break;
	case 9600:
		speed = B9600;
		break;
	case 19200:
		speed = B19200;
		break;
	case 38400:
		speed = B38400;
		break;
	}

	if (speed == 0) {
		log_warn("bad port baud rate: %d", baud_rate);
		return 0;
	}

	memcpy(&ttyset, &ttyset_old, sizeof(ttyset));

	ttyset.c_iflag = ttyset.c_oflag = 0;
	ttyset.c_cflag |= CS8 | CLOCAL | CREAD;
	ttyset.c_lflag = ~(ICANON | ISIG | ECHO);

	int i;
	for (i = 0; i < NCCS; i++)
		ttyset.c_cc[i] = -1;

	ttyset.c_cc[VMIN] = 1;

	/* unit: 1/10 second, max: 256
	 * NOTE: this also means message send rate must <= 25 seconds. */
	ttyset.c_cc[VTIME] = (U1)(TIMEOUT * 10);

	if (tcsetattr(gps_dev_fd, TCSANOW, &ttyset) != 0) {
		log_error("Unable to set UART attribute: %s", strerror(errno));
		return 0;
	}

	timeout.tv_sec = TIMEOUT; // NOTE
	timeout.tv_nsec = 0;

	fd_count = gps_dev_fd + 1;

	if (verify_output) {
		/* wait until UART has NMEA data output */
		char c;
		while (read(gps_dev_fd, &c, 1) < 0)
			sleep_ms(100);
	}

	/* turn back to blocking mode */
	flags &= ~O_NONBLOCK;
	if (fcntl(gps_dev_fd, F_SETFL, flags) != 0) {
		log_error("Unable to turn UART back to blocking mode: %s", strerror(errno));
		return 0;
	}

	return gps_dev_fd;
}
void uart_close()
{
	close(gps_dev_fd);
	gps_dev_fd = -1;
}

/**
 * select() may update the timeout argument to indicate how much time was left.
 * pselect() does not change this argument.
 * suspend/resume is the major reason that we use read/write with timeout.
 */
inline int read_with_timeout(U1 *buf, int len)
{
	FD_ZERO(&rs);
	FD_SET(gps_dev_fd, &rs);
	int ret = pselect(fd_count, &rs, NULL, NULL, &timeout, NULL);
	if (ret <= 0) {
		log_error("read UART failed: %s", ret == -1? strerror(errno) : "timeout");
		return ret;
	}

	if (FD_ISSET(gps_dev_fd, &rs)) {
		return read(gps_dev_fd, buf, len);
	} else {
		return -1;
	}
}

inline int write_with_timeout(U1 *buf, int len)
{
	FD_ZERO(&Ws);
	FD_SET(gps_dev_fd, &Ws);
	int ret = pselect(fd_count, NULL, &Ws, NULL, &timeout, NULL);
	if (ret <= 0) {
		log_error("write UART failed: %s", ret == -1? strerror(errno) : "timeout");
		return ret;
	}

	if (FD_ISSET(gps_dev_fd, &Ws)) {
		return write(gps_dev_fd, buf, len);
	} else {
		return -1;
	}
}

void uart_flush_output()
{
	tcflush(gps_dev_fd, TCOFLUSH);

	char buf[128];

	/* turn to non-blocking mode, consume (possible) ubx binary garbage */
	if (fcntl(gps_dev_fd, F_SETFL, flags|O_NONBLOCK) != 0) {
		log_error("sweep_garbage, change to non-blocking mode failed.");
		return;
	}

	/* important: wait for pending output to be written to GPS output buffer */
	sleep_ms(SEND_RATE);

	int count = 0;
	int n;

	while ((n = read(gps_dev_fd, buf, sizeof(buf))) > 0) {
		count += n;
		/* hack: lots of NMEA to read?
		 * size of any UBX output block should <= 1K */
		if (count > 1024) {
			log_warn("Detect possible conflict on UART.");
			g_context.uart_conflict = TRUE;
			break;
		}
	}

	/* restore flags */
	if (fcntl(gps_dev_fd, F_SETFL, flags) != 0) {
		log_error("sweep_garbage, change to blocking mode failed.");
		return;
	}
}

/*
 * If the read size < expected length, retry even if EOF
 */
gboolean inline read_fixed_len(U1 *buf, int expected_len)
{
	int len = 0, count;

	while (TRUE) {
		count = read_with_timeout(&buf[len], expected_len);
		if (count == expected_len) {
			return TRUE;
		} else if (count <= 0) {
			uart_flush_output();
			return FALSE;
		} else {
			len += count;
			expected_len -= count;
			if (expected_len == 0)
				return TRUE;
		}
	}
}
#endif

//////////////
#ifdef __linux__
void setserial(int fd,int closing_waitVal,int close_delay)//mill-second*10 (for On-Close??)
{
	struct serial_struct old_serinfo, new_serinfo;

	if (ioctl(fd, TIOCGSERIAL, &old_serinfo) < 0) {
		perror("Cannot get serial info");
		exit(1);
	}
	new_serinfo = old_serinfo;
#if 0	
	printf("old port=%d\n",new_serinfo.port);
	printf("old irq=%d\n",new_serinfo.irq);
	printf("old baudBase=%d\n",new_serinfo.baud_base);
	printf("old closing_wait=%d\n",new_serinfo.closing_wait);
	printf("old close_delay=%d\n",new_serinfo.close_delay);
#endif	
#define SETS_DIV 10
	//new_serinfo.port = port;
	//new_serinfo.irq = irq;
	new_serinfo.closing_wait=closing_waitVal/SETS_DIV;
	new_serinfo.close_delay=close_delay/SETS_DIV;
	new_serinfo.baud_base=115200;// prevent EINVAL
#if 0	
	printf("new closing_wait=%d\n",new_serinfo.closing_wait);
	printf("new close_delay=%d\n",new_serinfo.close_delay);
#endif	
	if (ioctl(fd, TIOCSSERIAL, &new_serinfo) < 0) {
		perror("Cannot set serial info");
		//exit(1);
	}
	if (ioctl(fd, TIOCGSERIAL, &new_serinfo) < 0) {
		perror("Cannot get serial info");
		exit(1);
	}
#if 0	
	printf("New port=%d\n",new_serinfo.port);
	printf("New irq=%d\n",new_serinfo.irq);
	printf("New closing_wait=%d\n",new_serinfo.closing_wait);
	printf("New close_delay=%d\n",new_serinfo.close_delay);
#endif	
	//printf("%s, Type: %s, Line: %d, "
	//	"Port: 0x%.4x (was 0x%.4x), IRQ: %d (was %d)\n",
	//	device, serial_type(new_serinfo.type),
	//	new_serinfo.line, new_serinfo.port, old_serinfo.port,
	//	new_serinfo.irq, old_serinfo.irq);
}
#endif
int g_lastUartNo	=0;
int /*vgwCardMan::*/UART_open_dev(int uartNo,const char* devNameInAndParam)//B115200, ~PARENB, ~CRTSCTS, CS8 | CLOCAL | CREAD, IGNPAR
{
    int     sd=-1;
    struct  termios  newtio;
	char devFileName[64];

	if(errno){
		fprintf(stderr,"error before UARTopen%d\n",errno);perror("open");
		errno=0;//auto clear error_no
	}
	if(uartNo>=0)
		sprintf(devFileName,TTY_NAME_FMT,uartNo);
	else
		sprintf(devFileName,devNameInAndParam,uartNo);
	printf("UART_open_dev: opening UART%d? dev==%s\n",uartNo,devFileName);
//    sd = open( devFileName, O_RDWR | O_NOCTTY | O_NDELAY);//| O_NONBLOCK//  | O_NDELAY
    sd = open( devFileName, O_RDWR |O_NOCTTY |O_NDELAY |O_NONBLOCK);//| O_NONBLOCK//  | O_NDELAY
    if( sd < 0 ) 
    {
        printf( "Serial[%s] Open Fail \r\n ",devFileName  );
        return -1;
    }
	if(errno){
		fprintf(stderr,"error after UARTopen%d on fd%d\n",errno,sd);perror("open");
		errno=0;//auto clear error_no
	}
#if 0
	tcgetattr( fd, &oldtio );               //save previous terminal state
 	memset( &newtio, 0, sizeof(newtio) );
 	newtio.c_cflag = B115200 | CS8 | CLOCAL | CREAD ;
 	newtio.c_iflag = IGNPAR;
 	newtio.c_oflag = OPOST | ONLCR;

 	//set input mode (non-canonical, no echo,.....)
 	newtio.c_lflag = 0;
	newtio.c_cc[VTIME] = 1; 	// 1 == 1/10 sec.
	newtio.c_cc[VMIN] = 255;// SERIAL_DATA_MAX; 
 	tcflush( fd, TCIOFLUSH );
 	tcsetattr( fd, TCSANOW, &newtio );
#else
    tcgetattr( sd, &oldtio );  
	if(errno){
		fprintf(stderr,"error after tcgetattr%d on fd%d\n",errno,sd);
		errno=0;//auto clear error_no
	}
    memset( &newtio, 0, sizeof(newtio) );
	cfsetispeed(&newtio,B115200);
	if(errno){
		fprintf(stderr,"error after cfsetispeed%d on fd%d\n",errno,sd);
		errno=0;//auto clear error_no
	}
	cfsetospeed(&newtio,B115200);
	if(errno){
		fprintf(stderr,"error after cfsetospeed%d on fd%d\n",errno,sd);
		errno=0;//auto clear error_no
	}
    newtio.c_cflag &= ~CRTSCTS;
    newtio.c_cflag &= ~PARENB; 
    newtio.c_cflag &= ~CSTOPB; 
    newtio.c_cflag &= ~CSIZE ; 
    newtio.c_cflag |= CS8 | CLOCAL | CREAD; 
    newtio.c_iflag = IGNPAR;//options.c_iflag &= ~(IXON | IXOFF | IXANY);
    newtio.c_oflag &= ~OPOST;//newtio.c_oflag = OPOST | ONLCR;//Choosing Raw Output
printf("c_lflag old is %x\n",    newtio.c_lflag);
    newtio.c_lflag = 0;//c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);//Choosing Raw Input
//    newtio.c_cc[VTIME] = 30;  // 1 == 1/10 sec.
//    newtio.c_cc[VMIN]  = 0;    // SERIAL_DATA_MAX; 
  /*
    모든 제어 문자들을 초기화한다.
    디폴트 값은 <termios.h> 헤더 파일에서 찾을 수 있다. 여기 comment에도
    추가로 달아놓았다.
  */
   newtio.c_cc[VINTR]    = 0;     /* Ctrl-c */
   newtio.c_cc[VQUIT]    = 0;     /* Ctrl-\ */
   newtio.c_cc[VERASE]   = 0;     /* del */
   newtio.c_cc[VKILL]    = 0;     /* @ */
#if 0   
   newtio.c_cc[VEOF]     = 4;     /* Ctrl-d */
   newtio.c_cc[VTIME]    = 10;//0;     /* inter-character timer unused */ //Timeout in deciseconds for non-canonical read
   newtio.c_cc[VMIN]     = 1;     /* blocking read until 1 character arrives */
#else   
   newtio.c_cc[VEOF]     = 0;     /* Ctrl-d */
/*
newtio.c_cc[VTIME] = 10; // Timeout in deciseconds for noncanonical read (TIME).
최소 1초 이상 수신이 없으면 타임 아웃이 걸린다. 이때 read 함수는 0을 반환한다.
newtio.c_cc[VMIN] = 32; // Minimum number of characters for noncanonical read (MIN).
VTIME 값이 0 일 경우 read문이 리턴되기 위한 최소의 수신 문자 개수를
지정한다.
MIN > 0, TIME = 0
MIN은 read가 리턴되기 위한 최소한의 문자 개수. TIME이 0이면 타이머는 사용되지
않는다.(무한대로 기다린다.)
MIN = 0, TIME > 0
TIME은 time-out 값으로 사용된다. Time-out 값은 TIME * 0.1 초이다. Time-out이
일어나기 전에 한 문자라도 들어오면 read는 리턴된다.
MIN > 0, TIME > 0
TIME은 time-out이 아닌 inter-character 타이머로 동작한다. 최소 MIN 개의 문자가
들어오거나 두 문자 사이의 시간이 TIME 값을 넘으면 리턴된다.
문자가 처음 들어올 때 타이머는 동작을 시작하고 이후 문자가 들어올 때마다 재
시작된다.
MIN = 0, TIME = 0
read는 즉시 리턴된다. 현재 읽을 수 있는 문자의 개수나 요청한 문자 개수가
반환된다. Antonino씨에 의하면 read하기 전에 fcntl(fd, F_SETFL, FNDELAY); 를
호출하면 똑같은 결과를 얻을 수 있다.
*/
   newtio.c_cc[VTIME]    = 0;// 10;//0;     /* inter-character timer unused */ //Timeout in deciseconds for non-canonical read
   newtio.c_cc[VMIN]     = 0;// 1     /* blocking read until 1 character arrives */
#endif   
   newtio.c_cc[VSWTC]    = 0;     /* '\0' */
   newtio.c_cc[VSTART]   = 0;     /* Ctrl-q */
   newtio.c_cc[VSTOP]    = 0;     /* Ctrl-s */
   newtio.c_cc[VSUSP]    = 0;     /* Ctrl-z */
   newtio.c_cc[VEOL]     = 0;     /* '\0' */
   newtio.c_cc[VREPRINT] = 0;     /* Ctrl-r */
   newtio.c_cc[VDISCARD] = 0;     /* Ctrl-u */
   newtio.c_cc[VWERASE]  = 0;     /* Ctrl-w */
   newtio.c_cc[VLNEXT]   = 0;     /* Ctrl-v */
   newtio.c_cc[VEOL2]    = 0;     /* '\0' */
    
    tcflush( sd, TCIFLUSH );
    tcsetattr( sd, TCSANOW, &newtio );

	fcntl(sd, F_SETFL, O_NONBLOCK);// socket_socket_nonblock(sd,1);
#if 0
	//fcntl(sd, F_SETFL, FNDELAY); 
	fcntl(sd, F_SETFL, FASYNC);
#endif	
	fcntl(sd, F_SETFL, FNDELAY);
	fcntl(sd, F_SETFL, fcntl(sd, F_GETFL, 0) | O_NONBLOCK); // - See more at: http://compgroups.net/comp.os.linux.development.apps/linux-equivalent-for-ioctlsock/2834646#sthash.iR2oJjHe.dpuf


#endif
	printf("UART_open_dev: UART%d_fd is %d\n",uartNo,sd);
	if(errno){
		fprintf(stderr,"error after UARTopenDev%d on fd%d\n",errno,sd);
		errno=0;//auto clear error_no
	}

	return sd;
}


void /*vgwCardMan::*/UART_close_dev(int sd)
{
printf("+UART_close_dev UART(fd=%d)\n",sd);
#if 0	// drop data!!
#define FLUSH_DELAY 3 // 3
    int fRet0=tcflush( sd, TCIOFLUSH );
	if(fRet0)printf("fRet0=%d\n",fRet0);
	sleep(FLUSH_DELAY);
    //tcflush( sd, TCIFLUSH );
    //tcflush( sd, TCOFLUSH );
    int fRet1=tcflush( sd, TCIOFLUSH );
	if(fRet1)printf("fRet1=%d\n",fRet1);

    tcsetattr( sd, TCSANOW, &oldtio );
    
    close( sd );   
#elif 1
#ifdef __linux__
#define FLUSH_DELAY 5 // 3
#define CLOSE_DELAY 50
	setserial(sd,FLUSH_DELAY*1000,CLOSE_DELAY);
#endif
//    tcsetattr( sd, TCSANOW, &oldtio );
    
#if 0	// re-open on no data // to test LAST write
	//safe_write(sd,"PONG\r\n",6+1);//UART_ADD_LEN
	safe_write(sd,"PONG\r\n",6);//UART_ADD_LEN
#else
#endif
	//time_t t0=time(NULL);	printf("beforeClose:%s",ctime(&t0));
    close( sd );   
	//time_t t1=time(NULL);	printf("afterClose:%s",ctime(&t1));
#endif	
printf("-UART_close_dev UART(fd=%d)\n",sd);
}

#define UART_DBG_LEVEL 5
#define UART_DBG_NBYTES 60
//#define UART_DBG_NBYTES 10
int g_PICmsgIdx=0,g_PICreqIdx=0;
bool /*vgwCardMan::*/OnSerialReceive(int sock,void *sock_ctx)
{
	char str[256*10*2],*pstr;
	int ret;
	int bytes=0;
#if 0	// re-open on no data	
#else
	int m_sd=sock;
#endif

	if(errno){
		fprintf(stderr,"fd(%d): error before FIONREAD  to fd errno%d\n",m_sd,errno);
		errno=0;//auto clear error_no
	}
	ret=ioctl(sock, FIONREAD, &bytes);
	if(errno){
		fprintf(stderr,"fd(%d): error FIONREAD to fd ret=%d errno%d,len=%d\n",m_sd,ret,errno,bytes);
		errno=0;//auto clear error_no
	}
	if(bytes<=0){
		return true;
	}
if(g_debugLevel>UART_DBG_LEVEL&&bytes>UART_DBG_NBYTES)printf("\n+UR%d\n",bytes);
	if(bytes>250){
		printf("SerialReceiveTooBig%d!!!\n",bytes);
		bytes=250;
	}
	str[0]='\0';
	ret=read(sock,str,bytes/*sizeof(str)*/);//m_sd
	if(errno){
		fprintf(stderr,"fd(%d): error uartRead to fd errno%d,len=%d(bytes=%d)\n",m_sd,errno,ret,bytes);
		errno=0;//auto clear error_no
	}
//if(g_debugLevel>UART_DBG_LEVEL)printf("-UR");
	if(ret>=0)str[ret]='\0';
	if(ret>0){
		int i;

		if(g_debugLevel>UART_DBG_LEVEL&&bytes>UART_DBG_NBYTES)printf("fromPIC32=\"%s\" ",str);
		if(g_debugLevel>UART_DBG_LEVEL&&bytes>UART_DBG_NBYTES)printf("(%d)\n",ret);
		for(i=0;i<ret;i++){
			static char m_sdRes[256*2];static int m_sdResIdx=0;
			
			//putchar(str[i]);
if(g_debugLevel>UART_DBG_LEVEL&&bytes>UART_DBG_NBYTES){if(i==0)printf("pre(%d) ",m_sdResIdx);printf("%02x ",str[i]);}
			if(m_sdResIdx<250&&str[i]){
				m_sdRes[m_sdResIdx++]=str[i];
				m_sdRes[m_sdResIdx]='\0';
				if(str[i]=='\n'){//or \n
					int errBefore=errno;
					char sdRes[256*2];

					if(errno){
						fprintf(stderr,"fd(%d): error (before OnSerialMessage) to fd errno%d,len=%d(bytes=%d)\n",m_sd,errno,ret,bytes);
						errno=0;//auto clear error_no
					}
					strcpy(sdRes,m_sdRes);m_sdRes[0]='\0';m_sdResIdx=0;
					// g_PICmsgIdx++;
					OnSerialMessage(sock,sdRes);
					if(errno){
						fprintf(stderr,"fd(%d): error (after OnSerialMessage) to fd errno%d,len=%d(bytes=%d)\n",m_sd,errno,ret,bytes);
						errno=0;//auto clear error_no
					}
				}
			}
			else printf("Ignore@%d(ch=0x%x)",m_sdResIdx,str[i]);
			if(g_debugLevel>UART_DBG_LEVEL&&bytes>UART_DBG_NBYTES&&i==ret-1)printf(" trail(%d) ",m_sdResIdx);
		}
	if(g_debugLevel>UART_DBG_LEVEL&&bytes>UART_DBG_NBYTES){printf("\n");}
	if(g_debugLevel>UART_DBG_LEVEL&&bytes>UART_DBG_NBYTES)fflush(stdout);
	}
	else if(ret==0){
		printf("\nno data.");fflush(stdout);
		//tcsendbreak(m_sd, 10);
#if 0	// re-open on no data		
			if(m_sd>0){
				KillTimer(VGWCARDMAN_PIC32CON_TID);
				ReadUnregisterSelfId(m_sd);
			}
			UART_close_dev(m_sd);
			m_sd=-1;
			m_sd=UART_open_dev(g_lastUartNo);
			if(m_sd>0){
				printf("m_sd is %d\n",m_sd);
				ReadRegisterSelfId(m_sd,NULL);
				SetTimer(ms2eloopsecs(VGWCARDMAN_PIC32CON_TIMEOUT1_VAL),VGWCARDMAN_PIC32CON_TID);
			}
			if(errno){
				fprintf(stderr,"fd(%d): error UART_close_dev/UART_open_dev to fd errno%d,len=%d(bytes=%d)\n",m_sd,errno,ret,bytes);
				errno=0;//auto clear error_no
			}
#endif			
	}
	else{
		//if(numErr%100==0)
		{
			int ErrNo=errno;
			printf("__%d(%lu)RD ret=%d,err%d\n",numErr++,time(NULL),ret,ErrNo);fflush(stdout);
		}
		if(errno){
			fprintf(stderr,"fd(%d): error RD Err to fd errno%d,len=%d\n",m_sd,errno,ret);
			errno=0;//auto clear error_no
		}
		return false;
	}
	return true;
}
#endif /*INCLUDE_UART2TCP_SUPPORT*/
