/*
 * wpa_supplicant/hostapd - Build time configuration defines
 * Copyright (c) 2005-2006, Jouni Malinen <j@w1.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 * See README and COPYING for more details.
 *
 * This header file can be used to define configuration defines that were
 * originally defined in Makefile. This is mainly meant for IDE use or for
 * systems that do not have suitable 'make' tool. In these cases, it may be
 * easier to have a single place for defining all the needed C pre-processor
 * defines.
 */

#ifndef BUILD_CONFIG_H
#define BUILD_CONFIG_H

/* ... Atheros: not used */
/*ELOOP*/
#ifndef WIN32
#undef CONFIG_NATIVE_WINDOWS
#define ELOOP_TIMECHANGE_PATCH 1
#define         ELOOP_TIMECHANGE_AUTO 1
#define         YWHWANG_PATCH_NTPNOTIFY 0
#else
#ifndef CONFIG_NATIVE_WINDOWS
#define CONFIG_NATIVE_WINDOWS
#endif
#define ELOOP_TIMECHANGE_PATCH 1
#define         ELOOP_TIMECHANGE_AUTO 0
#define         YWHWANG_PATCH_NTPNOTIFY 0
#endif
/*DNS&Async*/
#define YWHWANGPATCH_MULTIBSS_ACN 0

/*Feature Selections*/
#define HYWMON_SUPPORT_CTRLIF           0
#define HYWMON_SUPPORT_CTRLIF2           1
#ifndef WIN32
#define HYWMON_SUPPORT_SIGNALIF         0
#define HYWMON_SUPPORT_SIGNALIF2         1
#define HYWMON_SUPPORT_SYSLOGGING       1
#else
#define HYWMON_SUPPORT_SIGNALIF         0
#define HYWMON_SUPPORT_SIGNALIF2         0
#define HYWMON_SUPPORT_SYSLOGGING       0
#endif
#define HYWMON_SUPPORT_RELOAD           1
#define HYWMON_SUPPORT_HOSTAPDMON       0
#define HYWMON_SUPPORT_MEMORYMON        0
#define HYWMON_SUPPORT_WDTMON1           0
#define HYWMON_SUPPORT_WDTMON2           1
#define HYWMON_SUPPORT_WDTMON           1
#define         HYW_SW_WDT 0
#define         HYW_WDTID_DEFAULT1 1	/*for WDT*/
#define         HYW_WDTID_DEFAULT2 2	/*for clear*/
#define         HYW_WDTID_USERFIRST 3
#define HYWMON_SUPPORT_WANCHECK         1
#define HYWMON_SUPPORT_WLANCHECK        1
#define HYWMON_SUPPORT_PROCCHECK        1
#if HYWMON_SUPPORT_PROCCHECK
#define HYWMON_SUPPORT_SOCKSCHECK       0
#define HYWMON_SUPPORT_PROCMEMCHECK     0
#endif
#define HYWMON_SUPPORT_EVENTLOG         1

#if HYWMON_SUPPORT_CTRLIF
#define HYWMON_SUPPORT_DNSTEST          1 /*require dns.o & libtask.a*/
#define HYWMON_SUPPORT_EXCEPTTEST       1
#define HYWMON_SUPPORT_RESTARTTEST      1
#endif



struct hywmon_ctx {
	int dummy;
#if HYWMON_SUPPORT_WDTMON1
	int wdt_fd;
#endif
#if HYWMON_SUPPORT_CTRLIF
	int sock;
#endif
	void* pMegaco;
	void* pRTP;
	void* pScProc;
};

#endif /* BUILD_CONFIG_H */
