// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2007 - 2016 Realtek Corporation. All rights reserved. */

#if defined(CONFIG_MP_INCLUDED)

#include <drv_types.h>
#include <rtw_mp.h>
#include <rtw_mp_ioctl.h>
#include "phydm_precomp.h"

/*
 * Input Format: %s,%d,%d
 *	%s is width, could be
 *		"b" for 1 byte
 *		"w" for WORD (2 bytes)
 *		"dw" for DWORD (4 bytes)
 *	1st %d is address(offset)
 *	2st %d is data to write
 */
int rtw_mp_write_reg(struct net_device *dev,
		     struct iw_request_info *info,
		     struct iw_point *wrqu, char *extra)
{
	char *pch, *pnext, *ptmp;
	char *width_str;
	char width, buf[5];
	u32 addr, data;
	int ret;
	PADAPTER padapter = rtw_netdev_priv(dev);
	char input[128];

	if (wrqu->length > 128)
		return -EFAULT;
	if (copy_from_user(input, wrqu->pointer, wrqu->length))
		return -EFAULT;

	memset(extra, 0, wrqu->length);

	pch = input;

	pnext = strpbrk(pch, " ,.-");
	if (pnext == NULL)
		return -EINVAL;
	*pnext = 0;
	width_str = pch;

	pch = pnext + 1;
	pnext = strpbrk(pch, " ,.-");
	if (pnext == NULL)
		return -EINVAL;
	*pnext = 0;
	/*addr = simple_strtoul(pch, &ptmp, 16);
	memset(buf, '\0', sizeof(buf));
	memcpy(buf, pch, pnext-pch);
	ret = kstrtoul(buf, 16, &addr);*/
	ret = sscanf(pch, "%x", &addr);
	if (addr > 0x3FFF)
		return -EINVAL;

	pch = pnext + 1;
	pnext = strpbrk(pch, " ,.-");
	if ((pch - input) >= wrqu->length)
		return -EINVAL;
	/*data = simple_strtoul(pch, &ptmp, 16);*/
	ret = sscanf(pch, "%x", &data);
	RTW_INFO("data=%x,addr=%x\n", (u32)data, (u32)addr);
	ret = 0;
	width = width_str[0];
	switch (width) {
	case 'b':
		/* 1 byte*/
		if (data > 0xFF) {
			ret = -EINVAL;
			break;
		}
		rtw_write8(padapter, addr, data);
		break;
	case 'w':
		/* 2 bytes*/
		if (data > 0xFFFF) {
			ret = -EINVAL;
			break;
		}
		rtw_write16(padapter, addr, data);
		break;
	case 'd':
		/* 4 bytes*/
		rtw_write32(padapter, addr, data);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}


/*
 * Input Format: %s,%d
 *	%s is width, could be
 *		"b" for 1 byte
 *		"w" for WORD (2 bytes)
 *		"dw" for DWORD (4 bytes)
 *	%d is address(offset)
 *
 * Return:
 *	%d for data readed
 */
int rtw_mp_read_reg(struct net_device *dev,
		    struct iw_request_info *info,
		    struct iw_point *wrqu, char *extra)
{
	char input[128];
	char *pch, *pnext, *ptmp;
	char *width_str;
	char width;
	char data[20], tmp[20], buf[3];
	u32 addr = 0, strtout = 0;
	u32 i = 0, j = 0, ret = 0, data32 = 0;
	PADAPTER padapter = rtw_netdev_priv(dev);


	if (wrqu->length > 128)
		return -EFAULT;

	if (copy_from_user(input, wrqu->pointer, wrqu->length))
		return -EFAULT;

	memset(extra, 0, wrqu->length);
	memset(data, '\0', sizeof(data));
	memset(tmp, '\0', sizeof(tmp));
	pch = input;
	pnext = strpbrk(pch, " ,.-");
	if (pnext == NULL)
		return -EINVAL;
	*pnext = 0;
	width_str = pch;

	pch = pnext + 1;

	ret = sscanf(pch, "%x", &addr);
	if (addr > 0x3FFF)
		return -EINVAL;

	ret = 0;
	width = width_str[0];

	switch (width) {
	case 'b':
		data32 = rtw_read8(padapter, addr);
		RTW_INFO("%x\n", data32);
		sprintf(extra, "%d", data32);
		wrqu->length = strlen(extra);
		break;
	case 'w':
		/* 2 bytes*/
		sprintf(data, "%04x\n", rtw_read16(padapter, addr));

		for (i = 0 ; i <= strlen(data) ; i++) {
			if (i % 2 == 0) {
				tmp[j] = ' ';
				j++;
			}
			if (data[i] != '\0')
				tmp[j] = data[i];

			j++;
		}
		pch = tmp;
		RTW_INFO("pch=%s", pch);

		while (*pch != '\0') {
			pnext = strpbrk(pch, " ");
			if (!pnext || ((pnext - tmp) > 4))
				break;

			pnext++;
			if (*pnext != '\0') {
				/*strtout = simple_strtoul(pnext , &ptmp, 16);*/
				ret = sscanf(pnext, "%x", &strtout);
				sprintf(extra + strlen(extra), " %d", strtout);
			} else
				break;
			pch = pnext;
		}
		wrqu->length = strlen(extra);
		break;
	case 'd':
		/* 4 bytes */
		sprintf(data, "%08x", rtw_read32(padapter, addr));
		/*add read data format blank*/
		for (i = 0 ; i <= strlen(data) ; i++) {
			if (i % 2 == 0) {
				tmp[j] = ' ';
				j++;
			}
			if (data[i] != '\0')
				tmp[j] = data[i];

			j++;
		}
		pch = tmp;
		RTW_INFO("pch=%s", pch);

		while (*pch != '\0') {
			pnext = strpbrk(pch, " ");
			if (!pnext)
				break;

			pnext++;
			if (*pnext != '\0') {
				ret = sscanf(pnext, "%x", &strtout);
				sprintf(extra + strlen(extra), " %d", strtout);
			} else
				break;
			pch = pnext;
		}
		wrqu->length = strlen(extra);
		break;

	default:
		wrqu->length = 0;
		ret = -EINVAL;
		break;
	}

	return ret;
}


/*
 * Input Format: %d,%x,%x
 *	%d is RF path, should be smaller than MAX_RF_PATH_NUMS
 *	1st %x is address(offset)
 *	2st %x is data to write
 */
int rtw_mp_write_rf(struct net_device *dev,
		    struct iw_request_info *info,
		    struct iw_point *wrqu, char *extra)
{

	u32 path, addr, data;
	int ret;
	PADAPTER padapter = rtw_netdev_priv(dev);
	char input[128];

	if (wrqu->length > 128)
		return -EFAULT;
	memset(input, 0, wrqu->length);
	if (copy_from_user(input, wrqu->pointer, wrqu->length))
		return -EFAULT;


	ret = sscanf(input, "%d,%x,%x", &path, &addr, &data);
	if (ret < 3)
		return -EINVAL;

	if (path >= GET_HAL_RFPATH_NUM(padapter))
		return -EINVAL;
	if (addr > 0xFF)
		return -EINVAL;
	if (data > 0xFFFFF)
		return -EINVAL;

	memset(extra, 0, wrqu->length);

	write_rfreg(padapter, path, addr, data);

	sprintf(extra, "write_rf completed\n");
	wrqu->length = strlen(extra);

	return 0;
}


/*
 * Input Format: %d,%x
 *	%d is RF path, should be smaller than MAX_RF_PATH_NUMS
 *	%x is address(offset)
 *
 * Return:
 *	%d for data readed
 */
int rtw_mp_read_rf(struct net_device *dev,
		   struct iw_request_info *info,
		   struct iw_point *wrqu, char *extra)
{
	char input[128];
	char *pch, *pnext, *ptmp;
	char data[20], tmp[20], buf[3];
	u32 path, addr, strtou;
	u32 ret, i = 0 , j = 0;
	PADAPTER padapter = rtw_netdev_priv(dev);

	if (wrqu->length > 128)
		return -EFAULT;
	memset(input, 0, wrqu->length);
	if (copy_from_user(input, wrqu->pointer, wrqu->length))
		return -EFAULT;

	ret = sscanf(input, "%d,%x", &path, &addr);
	if (ret < 2)
		return -EINVAL;

	if (path >= GET_HAL_RFPATH_NUM(padapter))
		return -EINVAL;
	if (addr > 0xFF)
		return -EINVAL;

	memset(extra, 0, wrqu->length);

	sprintf(data, "%08x", read_rfreg(padapter, path, addr));
	/*add read data format blank*/
	for (i = 0 ; i <= strlen(data) ; i++) {
		if (i % 2 == 0) {
			tmp[j] = ' ';
			j++;
		}
		tmp[j] = data[i];
		j++;
	}
	pch = tmp;
	RTW_INFO("pch=%s", pch);

	while (*pch != '\0') {
		pnext = strpbrk(pch, " ");
		if (!pnext)
			break;
		pnext++;
		if (*pnext != '\0') {
			/*strtou =simple_strtoul(pnext , &ptmp, 16);*/
			ret = sscanf(pnext, "%x", &strtou);
			sprintf(extra + strlen(extra), " %d", strtou);
		} else
			break;
		pch = pnext;
	}
	wrqu->length = strlen(extra);

	return 0;
}


int rtw_mp_start(struct net_device *dev,
		 struct iw_request_info *info,
		 struct iw_point *wrqu, char *extra)
{
	int ret = 0;
	u8 val8;
	PADAPTER padapter = rtw_netdev_priv(dev);
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	struct hal_ops *pHalFunc = &padapter->hal_func;

	rtw_pm_set_ips(padapter, IPS_NONE);
	LeaveAllPowerSaveMode(padapter);

	if (rtw_mi_check_fwstate(padapter, _FW_UNDER_SURVEY))
		rtw_mi_scan_abort(padapter, false);

	if (rtw_mp_cmd(padapter, MP_START, RTW_CMDF_WAIT_ACK) != _SUCCESS)
		ret = -EPERM;

	memset(extra, 0, wrqu->length);
	sprintf(extra, "mp_start %s\n", ret == 0 ? "ok" : "fail");
	wrqu->length = strlen(extra);

	return ret;
}



int rtw_mp_stop(struct net_device *dev,
		struct iw_request_info *info,
		struct iw_point *wrqu, char *extra)
{
	int ret = 0;
	PADAPTER padapter = rtw_netdev_priv(dev);
	struct hal_ops *pHalFunc = &padapter->hal_func;

	if (rtw_mp_cmd(padapter, MP_STOP, RTW_CMDF_WAIT_ACK) != _SUCCESS)
		ret = -EPERM;

	memset(extra, 0, wrqu->length);
	sprintf(extra, "mp_stop %s\n", ret == 0 ? "ok" : "fail");
	wrqu->length = strlen(extra);

	return ret;
}


int rtw_mp_rate(struct net_device *dev,
		struct iw_request_info *info,
		struct iw_point *wrqu, char *extra)
{
	u32 rate = MPT_RATE_1M;
	u8		input[128];
	PADAPTER padapter = rtw_netdev_priv(dev);
	PMPT_CONTEXT		pMptCtx = &(padapter->mppriv.mpt_ctx);

	if (wrqu->length > 128)
		return -EFAULT;
	if (copy_from_user(input, wrqu->pointer, wrqu->length))
		return -EFAULT;

	rate = rtw_mpRateParseFunc(padapter, input);
	padapter->mppriv.rateidx = rate;

	if (rate == 0 && strcmp(input, "1M") != 0) {
		rate = rtw_atoi(input);
		padapter->mppriv.rateidx = MRateToHwRate(rate);
		/*if (rate <= 0x7f)
			rate = wifirate2_ratetbl_inx((u8)rate);
		else if (rate < 0xC8)
			rate = (rate - 0x79 + MPT_RATE_MCS0);
		HT  rate 0x80(MCS0)  ~ 0x8F(MCS15) ~ 0x9F(MCS31) 128~159
		VHT1SS~2SS rate 0xA0 (VHT1SS_MCS0 44) ~ 0xB3 (VHT2SS_MCS9 #63) 160~179
		VHT rate 0xB4 (VHT3SS_MCS0 64) ~ 0xC7 (VHT2SS_MCS9 #83) 180~199
		else
		VHT rate 0x90(VHT1SS_MCS0) ~ 0x99(VHT1SS_MCS9) 144~153
		rate =(rate - MPT_RATE_VHT1SS_MCS0);
		*/
	}
	memset(extra, 0, wrqu->length);

	sprintf(extra, "Set data rate to %s index %d" , input, padapter->mppriv.rateidx);
	RTW_INFO("%s: %s rate index=%d\n", __func__, input, padapter->mppriv.rateidx);

	if (padapter->mppriv.rateidx >= DESC_RATEVHTSS4MCS9)
		return -EINVAL;

	pMptCtx->mpt_rate_index = HwRateToMPTRate(padapter->mppriv.rateidx);
	SetDataRate(padapter);

	wrqu->length = strlen(extra);
	return 0;
}


int rtw_mp_channel(struct net_device *dev,
		   struct iw_request_info *info,
		   struct iw_point *wrqu, char *extra)
{

	PADAPTER padapter = rtw_netdev_priv(dev);
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(padapter);
	u8		input[128];
	u32	channel = 1;
	int cur_ch_offset;

	if (wrqu->length > 128)
		return -EFAULT;
	if (copy_from_user(input, wrqu->pointer, wrqu->length))
		return -EFAULT;

	channel = rtw_atoi(input);
	/*RTW_INFO("%s: channel=%d\n", __func__, channel);*/
	memset(extra, 0, wrqu->length);
	sprintf(extra, "Change channel %d to channel %d", padapter->mppriv.channel , channel);
	padapter->mppriv.channel = channel;
	SetChannel(padapter);
	pHalData->current_channel = channel;

	wrqu->length = strlen(extra);
	return 0;
}


int rtw_mp_bandwidth(struct net_device *dev,
		     struct iw_request_info *info,
		     struct iw_point *wrqu, char *extra)
{
	u32 bandwidth = 0, sg = 0;
	int cur_ch_offset;
	PADAPTER padapter = rtw_netdev_priv(dev);
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(padapter);
	u8		input[128];

	if (wrqu->length > 128)
		return -EFAULT;
	if (copy_from_user(input, wrqu->pointer, wrqu->length))
		return -EFAULT;

	if (sscanf(input, "40M=%d,shortGI=%d", &bandwidth, &sg) > 0)
		RTW_INFO("%s: bw=%d sg=%d\n", __func__, bandwidth , sg);

	if (bandwidth == 1)
		bandwidth = CHANNEL_WIDTH_40;
	else if (bandwidth == 2)
		bandwidth = CHANNEL_WIDTH_80;

	padapter->mppriv.bandwidth = (u8)bandwidth;
	padapter->mppriv.preamble = sg;
	memset(extra, 0, wrqu->length);
	sprintf(extra, "Change BW %d to BW %d\n", pHalData->current_channel_bw , bandwidth);

	SetBandwidth(padapter);
	pHalData->current_channel_bw = bandwidth;
	/*cur_ch_offset =  rtw_get_offset_by_ch(padapter->mppriv.channel);*/
	/*set_channel_bwmode(padapter, padapter->mppriv.channel, cur_ch_offset, bandwidth);*/
	wrqu->length = strlen(extra);

	return 0;
}


int rtw_mp_txpower_index(struct net_device *dev,
			 struct iw_request_info *info,
			 struct iw_point *wrqu, char *extra)
{
	PADAPTER padapter = rtw_netdev_priv(dev);
	char input[128];
	u32 rfpath;
	u32 txpower_inx;

	if (wrqu->length > 128)
		return -EFAULT;

	if (copy_from_user(input, wrqu->pointer, wrqu->length))
		return -EFAULT;

	rfpath = rtw_atoi(input);
	txpower_inx = mpt_ProQueryCalTxPower(padapter, rfpath);
	sprintf(extra, " %d", txpower_inx);
	wrqu->length = strlen(extra);

	return 0;
}


int rtw_mp_txpower(struct net_device *dev,
		   struct iw_request_info *info,
		   struct iw_point *wrqu, char *extra)
{
	u32 idx_a = 0, idx_b = 0, idx_c = 0, idx_d = 0, status = 0;
	int MsetPower = 1;
	u8		input[128];

	PADAPTER padapter = rtw_netdev_priv(dev);
	PMPT_CONTEXT		pMptCtx = &(padapter->mppriv.mpt_ctx);

	if (wrqu->length > 128)
		return -EFAULT;
	if (copy_from_user(input, wrqu->pointer, wrqu->length))
		return -EFAULT;

	MsetPower = strncmp(input, "off", 3);
	if (MsetPower == 0) {
		padapter->mppriv.bSetTxPower = 0;
		sprintf(extra, "MP Set power off");
	} else {
		if (sscanf(input, "patha=%d,pathb=%d,pathc=%d,pathd=%d", &idx_a, &idx_b, &idx_c, &idx_d) < 3)
			RTW_INFO("Invalid format on line %s ,patha=%d,pathb=%d,pathc=%d,pathd=%d\n", input , idx_a , idx_b , idx_c , idx_d);

		sprintf(extra, "Set power level path_A:%d path_B:%d path_C:%d path_D:%d", idx_a , idx_b , idx_c , idx_d);
		padapter->mppriv.txpoweridx = (u8)idx_a;

		pMptCtx->TxPwrLevel[ODM_RF_PATH_A] = (u8)idx_a;
		pMptCtx->TxPwrLevel[ODM_RF_PATH_B] = (u8)idx_b;
		pMptCtx->TxPwrLevel[ODM_RF_PATH_C] = (u8)idx_c;
		pMptCtx->TxPwrLevel[ODM_RF_PATH_D]  = (u8)idx_d;
		padapter->mppriv.bSetTxPower = 1;

		SetTxPower(padapter);
	}

	wrqu->length = strlen(extra);
	return 0;
}


int rtw_mp_ant_tx(struct net_device *dev,
		  struct iw_request_info *info,
		  struct iw_point *wrqu, char *extra)
{
	u8 i;
	u8		input[128];
	u16 antenna = 0;
	PADAPTER padapter = rtw_netdev_priv(dev);
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);

	if (wrqu->length > 128)
		return -EFAULT;
	if (copy_from_user(input, wrqu->pointer, wrqu->length))
		return -EFAULT;

	sprintf(extra, "switch Tx antenna to %s", input);

	for (i = 0; i < strlen(input); i++) {
		switch (input[i]) {
		case 'a':
			antenna |= ANTENNA_A;
			break;
		case 'b':
			antenna |= ANTENNA_B;
			break;
		case 'c':
			antenna |= ANTENNA_C;
			break;
		case 'd':
			antenna |= ANTENNA_D;
			break;
		}
	}
	/*antenna |= BIT(extra[i]-'a');*/
	RTW_INFO("%s: antenna=0x%x\n", __func__, antenna);
	padapter->mppriv.antenna_tx = antenna;
	padapter->mppriv.antenna_rx = antenna;
	/*RTW_INFO("%s:mppriv.antenna_rx=%d\n", __func__, padapter->mppriv.antenna_tx);*/
	pHalData->antenna_tx_path = antenna;

	SetAntenna(padapter);

	wrqu->length = strlen(extra);
	return 0;
}


int rtw_mp_ant_rx(struct net_device *dev,
		  struct iw_request_info *info,
		  struct iw_point *wrqu, char *extra)
{
	u8 i;
	u16 antenna = 0;
	u8		input[128];
	PADAPTER padapter = rtw_netdev_priv(dev);
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);

	if (wrqu->length > 128)
		return -EFAULT;
	if (copy_from_user(input, wrqu->pointer, wrqu->length))
		return -EFAULT;
	/*RTW_INFO("%s: input=%s\n", __func__, input);*/
	memset(extra, 0, wrqu->length);

	sprintf(extra, "switch Rx antenna to %s", input);

	for (i = 0; i < strlen(input); i++) {
		switch (input[i]) {
		case 'a':
			antenna |= ANTENNA_A;
			break;
		case 'b':
			antenna |= ANTENNA_B;
			break;
		case 'c':
			antenna |= ANTENNA_C;
			break;
		case 'd':
			antenna |= ANTENNA_D;
			break;
		}
	}

	RTW_INFO("%s: antenna=0x%x\n", __func__, antenna);
	padapter->mppriv.antenna_tx = antenna;
	padapter->mppriv.antenna_rx = antenna;
	pHalData->AntennaRxPath = antenna;
	/*RTW_INFO("%s:mppriv.antenna_rx=%d\n", __func__, padapter->mppriv.antenna_rx);*/
	SetAntenna(padapter);
	wrqu->length = strlen(extra);

	return 0;
}


int rtw_set_ctx_destAddr(struct net_device *dev,
			 struct iw_request_info *info,
			 struct iw_point *wrqu, char *extra)
{
	int jj, kk = 0;

	struct pkt_attrib *pattrib;
	struct mp_priv *pmp_priv;
	PADAPTER padapter = rtw_netdev_priv(dev);

	pmp_priv = &padapter->mppriv;
	pattrib = &pmp_priv->tx.attrib;

	if (strlen(extra) < 5)
		return _FAIL;

	RTW_INFO("%s: in=%s\n", __func__, extra);
	for (jj = 0, kk = 0; jj < ETH_ALEN; jj++, kk += 3)
		pattrib->dst[jj] = key_2char2num(extra[kk], extra[kk + 1]);

	RTW_INFO("pattrib->dst:%x %x %x %x %x %x\n", pattrib->dst[0], pattrib->dst[1], pattrib->dst[2], pattrib->dst[3], pattrib->dst[4], pattrib->dst[5]);
	return 0;
}



int rtw_mp_ctx(struct net_device *dev,
	       struct iw_request_info *info,
	       struct iw_point *wrqu, char *extra)
{
	u32 pkTx = 1;
	int countPkTx = 1, cotuTx = 1, CarrSprTx = 1, scTx = 1, sgleTx = 1, stop = 1;
	u32 bStartTest = 1;
	u32 count = 0, pktinterval = 0, pktlen = 0;
	u8 status;
	struct mp_priv *pmp_priv;
	struct pkt_attrib *pattrib;
	PADAPTER padapter = rtw_netdev_priv(dev);
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);

	pmp_priv = &padapter->mppriv;
	pattrib = &pmp_priv->tx.attrib;

	if (copy_from_user(extra, wrqu->pointer, wrqu->length))
		return -EFAULT;

	RTW_INFO("%s: in=%s\n", __func__, extra);
#ifdef CONFIG_CONCURRENT_MODE
	if (!is_primary_adapter(padapter)) {
		sprintf(extra, "Error: MP mode can't support Virtual Adapter, Please to use main Adapter.\n");
		wrqu->length = strlen(extra);
		return 0;
	}
#endif
	countPkTx = strncmp(extra, "count=", 5); /* strncmp true is 0*/
	cotuTx = strncmp(extra, "background", 20);
	CarrSprTx = strncmp(extra, "background,cs", 20);
	scTx = strncmp(extra, "background,sc", 20);
	sgleTx = strncmp(extra, "background,stone", 20);
	pkTx = strncmp(extra, "background,pkt", 20);
	stop = strncmp(extra, "stop", 4);
	if (sscanf(extra, "count=%d,pkt", &count) > 0)
		RTW_INFO("count= %d\n", count);
	if (sscanf(extra, "pktinterval=%d", &pktinterval) > 0)
		RTW_INFO("pktinterval= %d\n", pktinterval);

	if (sscanf(extra, "pktlen=%d", &pktlen) > 0)
		RTW_INFO("pktlen= %d\n", pktlen);

	if (!memcmp(extra, "destmac=", 8)) {
		wrqu->length -= 8;
		rtw_set_ctx_destAddr(dev, info, wrqu, &extra[8]);
		sprintf(extra, "Set dest mac OK !\n");
		return 0;
	}

	/*RTW_INFO("%s: count=%d countPkTx=%d cotuTx=%d CarrSprTx=%d scTx=%d sgleTx=%d pkTx=%d stop=%d\n", __func__, count, countPkTx, cotuTx, CarrSprTx, pkTx, sgleTx, scTx, stop);*/
	memset(extra, '\0', strlen(extra));

	if (pktinterval != 0) {
		sprintf(extra, "Pkt Interval = %d", pktinterval);
		padapter->mppriv.pktInterval = pktinterval;
		wrqu->length = strlen(extra);
		return 0;
	}
	if (pktlen != 0) {
		sprintf(extra, "Pkt len = %d", pktlen);
		pattrib->pktlen = pktlen;
		wrqu->length = strlen(extra);
		return 0;
	}
	if (stop == 0) {
		bStartTest = 0; /* To set Stop*/
		pmp_priv->tx.stop = 1;
		sprintf(extra, "Stop continuous Tx");
		odm_write_dig(&pHalData->odmpriv, 0x20);
	} else {
		bStartTest = 1;
		odm_write_dig(&pHalData->odmpriv, 0x7f);
		if (pmp_priv->mode != MP_ON) {
			if (pmp_priv->tx.stop != 1) {
				RTW_INFO("%s: MP_MODE != ON %d\n", __func__, pmp_priv->mode);
				return	-EFAULT;
			}
		}
	}

	pmp_priv->tx.count = count;

	if (pkTx == 0 || countPkTx == 0)
		pmp_priv->mode = MP_PACKET_TX;
	if (sgleTx == 0)
		pmp_priv->mode = MP_SINGLE_TONE_TX;
	if (cotuTx == 0)
		pmp_priv->mode = MP_CONTINUOUS_TX;
	if (CarrSprTx == 0)
		pmp_priv->mode = MP_CARRIER_SUPPRISSION_TX;
	if (scTx == 0)
		pmp_priv->mode = MP_SINGLE_CARRIER_TX;

	status = rtw_mp_pretx_proc(padapter, bStartTest, extra);

	wrqu->length = strlen(extra);
	return status;
}



int rtw_mp_disable_bt_coexist(struct net_device *dev,
			      struct iw_request_info *info,
			      union iwreq_data *wrqu, char *extra)
{
	PADAPTER padapter = (PADAPTER)rtw_netdev_priv(dev);
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	struct hal_ops *pHalFunc = &padapter->hal_func;

	u8 input[128];
	u32 bt_coexist;

	if (wrqu->data.length > 128)
		return -EFAULT;
	if (copy_from_user(input, wrqu->data.pointer, wrqu->data.length))
		return -EFAULT;

	bt_coexist = rtw_atoi(input);

	if (bt_coexist == 0) {
		RTW_INFO("Set OID_RT_SET_DISABLE_BT_COEXIST: disable BT_COEXIST\n");
#ifdef CONFIG_BT_COEXIST
		rtw_btcoex_HaltNotify(padapter);
		rtw_btcoex_SetManualControl(padapter, true);
		/* Force to switch Antenna to WiFi*/
		rtw_write16(padapter, 0x870, 0x300);
		rtw_write16(padapter, 0x860, 0x110);
#endif
		/* CONFIG_BT_COEXIST */
	} else {
#ifdef CONFIG_BT_COEXIST
		rtw_btcoex_SetManualControl(padapter, false);
#endif
	}

	return 0;
}


int rtw_mp_arx(struct net_device *dev,
	       struct iw_request_info *info,
	       struct iw_point *wrqu, char *extra)
{
	int bStartRx = 0, bStopRx = 0, bQueryPhy = 0, bQueryMac = 0, bSetBssid = 0;
	int bmac_filter = 0, bfilter_init = 0, bmon = 0, bSmpCfg = 0, bloopbk = 0;
	u8		input[128];
	char *pch, *ptmp, *token, *tmp[2] = {NULL, NULL};
	u32 i = 0, ii = 0, jj = 0, kk = 0, cnts = 0, ret;
	PADAPTER padapter = rtw_netdev_priv(dev);
	struct mp_priv *pmppriv = &padapter->mppriv;
	struct dbg_rx_counter rx_counter;

	if (wrqu->length > 128)
		return -EFAULT;
	if (copy_from_user(input, wrqu->pointer, wrqu->length))
		return -EFAULT;

	RTW_INFO("%s: %s\n", __func__, input);
#ifdef CONFIG_CONCURRENT_MODE
	if (!is_primary_adapter(padapter)) {
		sprintf(extra, "Error: MP mode can't support Virtual Adapter, Please to use main Adapter.\n");
		wrqu->length = strlen(extra);
		return 0;
	}
#endif
	bStartRx = (strncmp(input, "start", 5) == 0) ? 1 : 0; /* strncmp true is 0*/
	bStopRx = (strncmp(input, "stop", 5) == 0) ? 1 : 0; /* strncmp true is 0*/
	bQueryPhy = (strncmp(input, "phy", 3) == 0) ? 1 : 0; /* strncmp true is 0*/
	bQueryMac = (strncmp(input, "mac", 3) == 0) ? 1 : 0; /* strncmp true is 0*/
	bSetBssid = (strncmp(input, "setbssid=", 8) == 0) ? 1 : 0; /* strncmp true is 0*/
	/*bfilter_init = (strncmp(input, "filter_init",11)==0)?1:0;*/
	bmac_filter = (strncmp(input, "accept_mac", 10) == 0) ? 1 : 0;
	bmon = (strncmp(input, "mon=", 4) == 0) ? 1 : 0;
	bSmpCfg = (strncmp(input , "smpcfg=" , 7) == 0) ? 1 : 0;
	pmppriv->bloopback = (strncmp(input, "loopbk", 6) == 0) ? 1 : 0; /* strncmp true is 0*/

	if (bSetBssid == 1) {
		pch = input;
		while ((token = strsep(&pch, "=")) != NULL) {
			if (i > 1)
				break;
			tmp[i] = token;
			i++;
		}
		if ((tmp[0] != NULL) && (tmp[1] != NULL)) {
			cnts = strlen(tmp[1]) / 2;
			if (cnts < 1)
				return -EFAULT;
			RTW_INFO("%s: cnts=%d\n", __func__, cnts);
			RTW_INFO("%s: data=%s\n", __func__, tmp[1]);
			for (jj = 0, kk = 0; jj < cnts ; jj++, kk += 2) {
				pmppriv->network_macaddr[jj] = key_2char2num(tmp[1][kk], tmp[1][kk + 1]);
				RTW_INFO("network_macaddr[%d]=%x\n", jj, pmppriv->network_macaddr[jj]);
			}
		} else
			return -EFAULT;

		pmppriv->bSetRxBssid = true;
	}

	if (bmac_filter) {
		pmppriv->bmac_filter = bmac_filter;
		pch = input;
		while ((token = strsep(&pch, "=")) != NULL) {
			if (i > 1)
				break;
			tmp[i] = token;
			i++;
		}
		if ((tmp[0] != NULL) && (tmp[1] != NULL)) {
			cnts = strlen(tmp[1]) / 2;
			if (cnts < 1)
				return -EFAULT;
			RTW_INFO("%s: cnts=%d\n", __func__, cnts);
			RTW_INFO("%s: data=%s\n", __func__, tmp[1]);
			for (jj = 0, kk = 0; jj < cnts ; jj++, kk += 2) {
				pmppriv->mac_filter[jj] = key_2char2num(tmp[1][kk], tmp[1][kk + 1]);
				RTW_INFO("%s mac_filter[%d]=%x\n", __func__, jj, pmppriv->mac_filter[jj]);
			}
		} else
			return -EFAULT;

	}

	if (bStartRx) {
		sprintf(extra, "start");
		SetPacketRx(padapter, bStartRx, false);
	} else if (bStopRx) {
		SetPacketRx(padapter, bStartRx, false);
		pmppriv->bmac_filter = false;
		pmppriv->bSetRxBssid = false;
		sprintf(extra, "Received packet OK:%d CRC error:%d ,Filter out:%d", padapter->mppriv.rx_pktcount, padapter->mppriv.rx_crcerrpktcount, padapter->mppriv.rx_pktcount_filter_out);
	} else if (bQueryPhy) {
		memset(&rx_counter, 0, sizeof(struct dbg_rx_counter));
		rtw_dump_phy_rx_counters(padapter, &rx_counter);

		RTW_INFO("%s: OFDM_FA =%d\n", __func__, rx_counter.rx_ofdm_fa);
		RTW_INFO("%s: CCK_FA =%d\n", __func__, rx_counter.rx_cck_fa);
		sprintf(extra, "Phy Received packet OK:%d CRC error:%d FA Counter: %d", rx_counter.rx_pkt_ok, rx_counter.rx_pkt_crc_error, rx_counter.rx_cck_fa + rx_counter.rx_ofdm_fa);


	} else if (bQueryMac) {
		memset(&rx_counter, 0, sizeof(struct dbg_rx_counter));
		rtw_dump_mac_rx_counters(padapter, &rx_counter);
		sprintf(extra, "Mac Received packet OK: %d , CRC error: %d , Drop Packets: %d\n",
			rx_counter.rx_pkt_ok, rx_counter.rx_pkt_crc_error, rx_counter.rx_pkt_drop);

	}

	if (bmon == 1) {
		ret = sscanf(input, "mon=%d", &bmon);

		if (bmon == 1) {
			pmppriv->rx_bindicatePkt = true;
			sprintf(extra, "Indicating Receive Packet to network start\n");
		} else {
			pmppriv->rx_bindicatePkt = false;
			sprintf(extra, "Indicating Receive Packet to network Stop\n");
		}
	}
	if (bSmpCfg == 1) {
		ret = sscanf(input, "smpcfg=%d", &bSmpCfg);

		if (bSmpCfg == 1) {
			pmppriv->bRTWSmbCfg = true;
			sprintf(extra , "Indicate By Simple Config Format\n");
			SetPacketRx(padapter, true, true);
		} else {
			pmppriv->bRTWSmbCfg = false;
			sprintf(extra , "Indicate By Normal Format\n");
			SetPacketRx(padapter, true, false);
		}
	}

	if (pmppriv->bloopback == true) {
		sprintf(extra , "Enter MAC LoopBack mode\n");
		_rtw_write32(padapter, 0x100, 0xB0106FF);
		RTW_INFO("0x100 :0x%x" , _rtw_read32(padapter, 0x100));
		_rtw_write16(padapter, 0x608, 0x30c);
		RTW_INFO("0x100 :0x%x" , _rtw_read32(padapter, 0x608));
	}

	wrqu->length = strlen(extra) + 1;

	return 0;
}


int rtw_mp_trx_query(struct net_device *dev,
		     struct iw_request_info *info,
		     struct iw_point *wrqu, char *extra)
{
	u32 txok, txfail, rxok, rxfail, rxfilterout;
	PADAPTER padapter = rtw_netdev_priv(dev);
	PMPT_CONTEXT	pMptCtx		=	&(padapter->mppriv.mpt_ctx);
	RT_PMAC_TX_INFO	PMacTxInfo	=	pMptCtx->PMacTxInfo;

	if (PMacTxInfo.bEnPMacTx == true)
		txok = hal_mpt_query_phytxok(padapter);
	else
		txok = padapter->mppriv.tx.sended;

	txfail = 0;
	rxok = padapter->mppriv.rx_pktcount;
	rxfail = padapter->mppriv.rx_crcerrpktcount;
	rxfilterout = padapter->mppriv.rx_pktcount_filter_out;

	memset(extra, '\0', 128);

	sprintf(extra, "Tx OK:%d, Tx Fail:%d, Rx OK:%d, CRC error:%d ,Rx Filter out:%d\n", txok, txfail, rxok, rxfail, rxfilterout);

	wrqu->length = strlen(extra) + 1;

	return 0;
}


int rtw_mp_pwrtrk(struct net_device *dev,
		  struct iw_request_info *info,
		  struct iw_point *wrqu, char *extra)
{
	u8 enable;
	u32 thermal;
	s32 ret;
	PADAPTER padapter = rtw_netdev_priv(dev);
	HAL_DATA_TYPE			*pHalData = GET_HAL_DATA(padapter);
	u8		input[128];

	if (wrqu->length > 128)
		return -EFAULT;
	if (copy_from_user(input, wrqu->pointer, wrqu->length))
		return -EFAULT;

	memset(extra, 0, wrqu->length);

	enable = 1;
	if (wrqu->length > 1) {
		/* not empty string*/
		if (strncmp(input, "stop", 4) == 0) {
			enable = 0;
			sprintf(extra, "mp tx power tracking stop");
		} else if (sscanf(input, "ther=%d", &thermal) == 1) {
			ret = SetThermalMeter(padapter, (u8)thermal);
			if (ret == _FAIL)
				return -EPERM;
			sprintf(extra, "mp tx power tracking start,target value=%d ok", thermal);
		} else
			return -EINVAL;
	}

	ret = SetPowerTracking(padapter, enable);
	if (ret == _FAIL)
		return -EPERM;

	wrqu->length = strlen(extra);

	return 0;
}



int rtw_mp_psd(struct net_device *dev,
	       struct iw_request_info *info,
	       struct iw_point *wrqu, char *extra)
{
	PADAPTER padapter = rtw_netdev_priv(dev);
	u8		input[128];

	if (wrqu->length > 128)
		return -EFAULT;
	if (copy_from_user(input, wrqu->pointer, wrqu->length))
		return -EFAULT;

	strcpy(extra, input);

	wrqu->length = mp_query_psd(padapter, extra);

	return 0;
}


int rtw_mp_thermal(struct net_device *dev,
		   struct iw_request_info *info,
		   struct iw_point *wrqu, char *extra)
{
	u8 val;
	int bwrite = 1;
	u16 addr = EEPROM_THERMAL_METER_88E;
	u16 cnt = 1;
	u16 max_available_size = 0;
	PADAPTER padapter = rtw_netdev_priv(dev);

	if (copy_from_user(extra, wrqu->pointer, wrqu->length))
		return -EFAULT;

	bwrite = strncmp(extra, "write", 6);/* strncmp true is 0*/

	GetThermalMeter(padapter, &val);

	if (bwrite == 0) {
		/*RTW_INFO("to write val:%d",val);*/
		EFUSE_GetEfuseDefinition(padapter, EFUSE_WIFI, TYPE_AVAILABLE_EFUSE_BYTES_TOTAL, (void *)&max_available_size, false);
		if (2 > max_available_size) {
			RTW_INFO("no available efuse!\n");
			return -EFAULT;
		}
		if (rtw_efuse_map_write(padapter, addr, cnt, &val) == _FAIL) {
			RTW_INFO("rtw_efuse_map_write error\n");
			return -EFAULT;
		}
		sprintf(extra, " efuse write ok :%d", val);
	} else
		sprintf(extra, "%d", val);
	wrqu->length = strlen(extra);

	return 0;
}



int rtw_mp_reset_stats(struct net_device *dev,
		       struct iw_request_info *info,
		       struct iw_point *wrqu, char *extra)
{
	struct mp_priv *pmp_priv;
	struct pkt_attrib *pattrib;
	PADAPTER padapter = rtw_netdev_priv(dev);

	pmp_priv = &padapter->mppriv;

	pmp_priv->tx.sended = 0;
	pmp_priv->tx_pktcount = 0;
	pmp_priv->rx_pktcount = 0;
	pmp_priv->rx_pktcount_filter_out = 0;
	pmp_priv->rx_crcerrpktcount = 0;

	rtw_reset_phy_rx_counters(padapter);
	rtw_reset_mac_rx_counters(padapter);

	memset(extra, 0, wrqu->length);
	sprintf(extra, "mp_reset_stats ok\n");
	wrqu->length = strlen(extra);

	return 0;
}


int rtw_mp_dump(struct net_device *dev,
		struct iw_request_info *info,
		struct iw_point *wrqu, char *extra)
{
	struct mp_priv *pmp_priv;
	struct pkt_attrib *pattrib;
	u32 value;
	u8		input[128];
	u8 rf_type, path_nums = 0;
	u32 i, j = 1, path;
	PADAPTER padapter = rtw_netdev_priv(dev);

	pmp_priv = &padapter->mppriv;

	if (wrqu->length > 128)
		return -EFAULT;
	if (copy_from_user(input, wrqu->pointer, wrqu->length))
		return -EFAULT;

	if (strncmp(input, "all", 4) == 0) {
		mac_reg_dump(RTW_DBGDUMP, padapter);
		bb_reg_dump(RTW_DBGDUMP, padapter);
		rf_reg_dump(RTW_DBGDUMP, padapter);
	}
	return 0;
}


int rtw_mp_phypara(struct net_device *dev,
		   struct iw_request_info *info,
		   struct iw_point *wrqu, char *extra)
{

	PADAPTER padapter = rtw_netdev_priv(dev);
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(padapter);
	char	input[128];
	u32		valxcap, ret;

	if (wrqu->length > 128)
		return -EFAULT;
	if (copy_from_user(input, wrqu->pointer, wrqu->length))
		return -EFAULT;

	RTW_INFO("%s:iwpriv in=%s\n", __func__, input);

	ret = sscanf(input, "xcap=%d", &valxcap);

	pHalData->crystal_cap = (u8)valxcap;
	hal_set_crystal_cap(padapter , valxcap);

	sprintf(extra, "Set xcap=%d", valxcap);
	wrqu->length = strlen(extra) + 1;

	return 0;

}


int rtw_mp_SetRFPath(struct net_device *dev,
		     struct iw_request_info *info,
		     struct iw_point *wrqu, char *extra)
{
	PADAPTER padapter = rtw_netdev_priv(dev);
	char	input[128];
	int		bMain = 1, bTurnoff = 1;

	if (wrqu->length > 128)
		return -EFAULT;
	RTW_INFO("%s:iwpriv in=%s\n", __func__, input);

	if (copy_from_user(input, wrqu->pointer, wrqu->length))
		return -EFAULT;

	bMain = strncmp(input, "1", 2); /* strncmp true is 0*/
	bTurnoff = strncmp(input, "0", 3); /* strncmp true is 0*/

	memset(extra, 0, wrqu->length);

	if (bMain == 0) {
		MP_PHY_SetRFPathSwitch(padapter, true);
		RTW_INFO("%s:PHY_SetRFPathSwitch=true\n", __func__);
		sprintf(extra, "mp_setrfpath Main\n");

	} else if (bTurnoff == 0) {
		MP_PHY_SetRFPathSwitch(padapter, false);
		RTW_INFO("%s:PHY_SetRFPathSwitch=false\n", __func__);
		sprintf(extra, "mp_setrfpath Aux\n");
	} else {
		bMain = MP_PHY_QueryRFPathSwitch(padapter);
		RTW_INFO("%s:PHY_SetRFPathSwitch = %s\n", __func__, (bMain ? "Main":"Aux"));
		sprintf(extra, "mp_setrfpath %s\n" , (bMain ? "Main":"Aux"));
	}

	wrqu->length = strlen(extra);

	return 0;
}


int rtw_mp_QueryDrv(struct net_device *dev,
		    struct iw_request_info *info,
		    union iwreq_data *wrqu, char *extra)
{
	PADAPTER padapter = rtw_netdev_priv(dev);
	char	input[128];
	int	qAutoLoad = 1;
	PHAL_DATA_TYPE pHalData = GET_HAL_DATA(padapter);

	if (wrqu->data.length > 128)
		return -EFAULT;
	if (copy_from_user(input, wrqu->data.pointer, wrqu->data.length))
		return -EFAULT;
	RTW_INFO("%s:iwpriv in=%s\n", __func__, input);

	qAutoLoad = strncmp(input, "autoload", 8); /* strncmp true is 0*/

	if (qAutoLoad == 0) {
		RTW_INFO("%s:qAutoLoad\n", __func__);

		if (pHalData->bautoload_fail_flag)
			sprintf(extra, "fail");
		else
			sprintf(extra, "ok");
	}
	wrqu->data.length = strlen(extra) + 1;
	return 0;
}


int rtw_mp_PwrCtlDM(struct net_device *dev,
		    struct iw_request_info *info,
		    struct iw_point *wrqu, char *extra)
{
	PADAPTER padapter = rtw_netdev_priv(dev);
	u8		input[128];
	int		bstart = 1;

	if (wrqu->length > 128)
		return -EFAULT;
	if (copy_from_user(input, wrqu->pointer, wrqu->length))
		return -EFAULT;

	bstart = strncmp(input, "start", 5); /* strncmp true is 0*/
	if (bstart == 0) {
		sprintf(extra, "PwrCtlDM start\n");
		MPT_PwrCtlDM(padapter, 1);
	} else {
		sprintf(extra, "PwrCtlDM stop\n");
		MPT_PwrCtlDM(padapter, 0);
	}
	wrqu->length = strlen(extra);

	return 0;
}

int rtw_mp_iqk(struct net_device *dev,
		 struct iw_request_info *info,
		 struct iw_point *wrqu, char *extra)
{
	PADAPTER padapter = rtw_netdev_priv(dev);

	rtw_mp_trigger_iqk(padapter);

	return 0;
}

int rtw_mp_lck(struct net_device *dev,
		 struct iw_request_info *info,
		 struct iw_point *wrqu, char *extra)
{
	PADAPTER padapter = rtw_netdev_priv(dev);

	rtw_mp_trigger_lck(padapter);

	return 0;
}

int rtw_mp_getver(struct net_device *dev,
		  struct iw_request_info *info,
		  union iwreq_data *wrqu, char *extra)
{
	PADAPTER padapter = rtw_netdev_priv(dev);
	struct mp_priv *pmp_priv;

	pmp_priv = &padapter->mppriv;

	if (copy_from_user(extra, wrqu->data.pointer, wrqu->data.length))
		return -EFAULT;

	sprintf(extra, "rtwpriv=%d\n", RTWPRIV_VER_INFO);
	wrqu->data.length = strlen(extra);
	return 0;
}


int rtw_mp_mon(struct net_device *dev,
	       struct iw_request_info *info,
	       union iwreq_data *wrqu, char *extra)
{
	PADAPTER padapter = rtw_netdev_priv(dev);
	struct mp_priv *pmp_priv = &padapter->mppriv;
	struct mlme_priv *pmlmepriv = &padapter->mlmepriv;
	struct hal_ops *pHalFunc = &padapter->hal_func;
	NDIS_802_11_NETWORK_INFRASTRUCTURE networkType;
	int bstart = 1, bstop = 1;

	networkType = Ndis802_11Infrastructure;
	if (copy_from_user(extra, wrqu->data.pointer, wrqu->data.length))
		return -EFAULT;

	rtw_pm_set_ips(padapter, IPS_NONE);
	LeaveAllPowerSaveMode(padapter);

#ifdef CONFIG_MP_INCLUDED
	if (init_mp_priv(padapter) == _FAIL)
		RTW_INFO("%s: initialize MP private data Fail!\n", __func__);
	padapter->mppriv.channel = 6;

	bstart = strncmp(extra, "start", 5); /* strncmp true is 0*/
	bstop = strncmp(extra, "stop", 4); /* strncmp true is 0*/
	if (bstart == 0) {
		mp_join(padapter, WIFI_FW_ADHOC_STATE);
		SetPacketRx(padapter, true, false);
		SetChannel(padapter);
		pmp_priv->rx_bindicatePkt = true;
		pmp_priv->bRTWSmbCfg = true;
		sprintf(extra, "monitor mode start\n");
	} else if (bstop == 0) {
		SetPacketRx(padapter, false, false);
		pmp_priv->rx_bindicatePkt = false;
		pmp_priv->bRTWSmbCfg = false;
		padapter->registrypriv.mp_mode = 1;
		pHalFunc->hal_deinit(padapter);
		padapter->registrypriv.mp_mode = 0;
		pHalFunc->hal_init(padapter);
		/*rtw_disassoc_cmd(padapter, 0, true);*/
		if (check_fwstate(pmlmepriv, _FW_LINKED) == true) {
			rtw_disassoc_cmd(padapter, 500, true);
			rtw_indicate_disconnect(padapter, 0, false);
			/*rtw_free_assoc_resources(padapter, 1);*/
		}
		rtw_pm_set_ips(padapter, IPS_NORMAL);
		sprintf(extra, "monitor mode Stop\n");
	}
#endif
	wrqu->data.length = strlen(extra);
	return 0;
}

int rtw_mp_pretx_proc(PADAPTER padapter, u8 bStartTest, char *extra)
{
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(padapter);
	struct mp_priv *pmp_priv = &padapter->mppriv;
	PMPT_CONTEXT		pMptCtx = &(padapter->mppriv.mpt_ctx);

	switch (pmp_priv->mode) {

	case MP_PACKET_TX:
		if (bStartTest == 0) {
			pmp_priv->tx.stop = 1;
			pmp_priv->mode = MP_ON;
			sprintf(extra, "Stop continuous Tx");
		} else if (pmp_priv->tx.stop == 1) {
			sprintf(extra + strlen(extra), "\nStart continuous DA=ffffffffffff len=1500 count=%u\n", pmp_priv->tx.count);
			pmp_priv->tx.stop = 0;
			SetPacketTx(padapter);
		} else
			return -EFAULT;
		return 0;
	case MP_SINGLE_TONE_TX:
		if (bStartTest != 0)
			sprintf(extra + strlen(extra), "\nStart continuous DA=ffffffffffff len=1500\n infinite=yes.");
		SetSingleToneTx(padapter, (u8)bStartTest);
		break;
	case MP_CONTINUOUS_TX:
		if (bStartTest != 0)
			sprintf(extra + strlen(extra), "\nStart continuous DA=ffffffffffff len=1500\n infinite=yes.");
		SetContinuousTx(padapter, (u8)bStartTest);
		break;
	case MP_CARRIER_SUPPRISSION_TX:
		if (bStartTest != 0) {
			if (HwRateToMPTRate(pmp_priv->rateidx) <= MPT_RATE_11M)
				sprintf(extra + strlen(extra), "\nStart continuous DA=ffffffffffff len=1500\n infinite=yes.");
			else
				sprintf(extra + strlen(extra), "\nSpecify carrier suppression but not CCK rate");
		}
		SetCarrierSuppressionTx(padapter, (u8)bStartTest);
		break;
	case MP_SINGLE_CARRIER_TX:
		if (bStartTest != 0)
			sprintf(extra + strlen(extra), "\nStart continuous DA=ffffffffffff len=1500\n infinite=yes.");
		SetSingleCarrierTx(padapter, (u8)bStartTest);
		break;

	default:
		sprintf(extra, "Error! Continuous-Tx is not on-going.");
		return -EFAULT;
	}

	if (bStartTest == 1 && pmp_priv->mode != MP_ON) {
		struct mp_priv *pmp_priv = &padapter->mppriv;

		if (pmp_priv->tx.stop == 0) {
			pmp_priv->tx.stop = 1;
			rtw_msleep_os(5);
		}
		pmp_priv->tx.attrib.ht_en = 1;
		pmp_priv->tx.stop = 0;
		pmp_priv->tx.count = 1;
		SetPacketTx(padapter);
	} else
		pmp_priv->mode = MP_ON;

	return 0;
}

int rtw_mp_tx(struct net_device *dev,
	      struct iw_request_info *info,
	      union iwreq_data *wrqu, char *extra)
{
	PADAPTER padapter = rtw_netdev_priv(dev);
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(padapter);
	struct mp_priv *pmp_priv = &padapter->mppriv;
	PMPT_CONTEXT		pMptCtx = &(padapter->mppriv.mpt_ctx);
	struct registry_priv	*pregistrypriv = &padapter->registrypriv;

	u32 bandwidth = 0, sg = 0, channel = 6, txpower = 40, rate = 108, ant = 0, txmode = 1, count = 0;
	u8 i = 0, j = 0, bStartTest = 1, status = 0, Idx = 0, tmpU1B = 0;
	u16 antenna = 0;

	if (copy_from_user(extra, wrqu->data.pointer, wrqu->data.length))
		return -EFAULT;
	RTW_INFO("extra = %s\n", extra);
#ifdef CONFIG_CONCURRENT_MODE
	if (!is_primary_adapter(padapter)) {
		sprintf(extra, "Error: MP mode can't support Virtual Adapter, Please to use main Adapter.\n");
		wrqu->data.length = strlen(extra);
		return 0;
	}
#endif

	if (strncmp(extra, "stop", 3) == 0) {
		bStartTest = 0; /* To set Stop*/
		pmp_priv->tx.stop = 1;
		sprintf(extra, "Stop continuous Tx");
		status = rtw_mp_pretx_proc(padapter, bStartTest, extra);
		wrqu->data.length = strlen(extra);
		return status;
	} else if (strncmp(extra, "count", 5) == 0) {
		if (sscanf(extra, "count=%d", &count) < 1)
			RTW_INFO("Got Count=%d]\n", count);
		pmp_priv->tx.count = count;
		return 0;
	} else if (strncmp(extra, "setting", 7) == 0) {
		memset(extra, 0, wrqu->data.length);
		sprintf(extra, "Current Setting :\n Channel:%d", pmp_priv->channel);
		sprintf(extra + strlen(extra), "\n Bandwidth:%d", pmp_priv->bandwidth);
		sprintf(extra + strlen(extra), "\n Rate index:%d", pmp_priv->rateidx);
		sprintf(extra + strlen(extra), "\n TxPower index:%d", pmp_priv->txpoweridx);
		sprintf(extra + strlen(extra), "\n Antenna TxPath:%d", pmp_priv->antenna_tx);
		sprintf(extra + strlen(extra), "\n Antenna RxPath:%d", pmp_priv->antenna_rx);
		sprintf(extra + strlen(extra), "\n MP Mode:%d", pmp_priv->mode);
		wrqu->data.length = strlen(extra);
		return 0;
#ifdef CONFIG_MP_VHT_HW_TX_MODE
	} else if (strncmp(extra, "pmact", 5) == 0) {
		if (strncmp(extra, "pmact=", 6) == 0) {
			memset(&pMptCtx->PMacTxInfo, 0, sizeof(pMptCtx->PMacTxInfo));
			if (strncmp(extra, "pmact=start", 11) == 0) {
				pMptCtx->PMacTxInfo.bEnPMacTx = true;
				sprintf(extra, "Set PMac Tx Mode start\n");
			} else {
				pMptCtx->PMacTxInfo.bEnPMacTx = false;
				sprintf(extra, "Set PMac Tx Mode Stop\n");
			}
			if (pMptCtx->bldpc == true)
				pMptCtx->PMacTxInfo.bLDPC = true;

			if (pMptCtx->bstbc == true)
				pMptCtx->PMacTxInfo.bSTBC = true;

			pMptCtx->PMacTxInfo.bSPreamble = pmp_priv->preamble;
			pMptCtx->PMacTxInfo.bSGI = pmp_priv->preamble;
			pMptCtx->PMacTxInfo.BandWidth = pmp_priv->bandwidth;
			pMptCtx->PMacTxInfo.TX_RATE = HwRateToMPTRate(pmp_priv->rateidx);

			pMptCtx->PMacTxInfo.Mode = pMptCtx->HWTxmode;

			pMptCtx->PMacTxInfo.NDP_sound = false;/*(Adapter.PacketType == NDP_PKT)?true:false;*/

			if (padapter->mppriv.pktInterval == 0)
				pMptCtx->PMacTxInfo.PacketPeriod = 100;
			else
				pMptCtx->PMacTxInfo.PacketPeriod = padapter->mppriv.pktInterval;

			if (padapter->mppriv.pktLength < 1000)
				pMptCtx->PMacTxInfo.PacketLength = 1000;
			else
				pMptCtx->PMacTxInfo.PacketLength = padapter->mppriv.pktLength;

			pMptCtx->PMacTxInfo.PacketPattern  = rtw_random32() % 0xFF;

			if (padapter->mppriv.tx_pktcount != 0)
				pMptCtx->PMacTxInfo.PacketCount = padapter->mppriv.tx_pktcount;

			pMptCtx->PMacTxInfo.Ntx = 0;
			for (Idx = 16; Idx < 20; Idx++) {
				tmpU1B = (padapter->mppriv.antenna_tx >> Idx) & 1;
				if (tmpU1B)
					pMptCtx->PMacTxInfo.Ntx++;
			}

			memset(pMptCtx->PMacTxInfo.MacAddress, 0xFF, ETH_ALEN);

			PMAC_Get_Pkt_Param(&pMptCtx->PMacTxInfo, &pMptCtx->PMacPktInfo);

			if (MPT_IS_CCK_RATE(pMptCtx->PMacTxInfo.TX_RATE))

				CCK_generator(&pMptCtx->PMacTxInfo, &pMptCtx->PMacPktInfo);
			else {
				PMAC_Nsym_generator(&pMptCtx->PMacTxInfo, &pMptCtx->PMacPktInfo);
				/* 24 BIT*/
				L_SIG_generator(pMptCtx->PMacPktInfo.N_sym, &pMptCtx->PMacTxInfo, &pMptCtx->PMacPktInfo);
			}
			/*	48BIT*/
			if (MPT_IS_HT_RATE(pMptCtx->PMacTxInfo.TX_RATE))
				HT_SIG_generator(&pMptCtx->PMacTxInfo, &pMptCtx->PMacPktInfo);
			else if (MPT_IS_VHT_RATE(pMptCtx->PMacTxInfo.TX_RATE)) {
				/*	48BIT*/
				VHT_SIG_A_generator(&pMptCtx->PMacTxInfo, &pMptCtx->PMacPktInfo);

				/*	26/27/29 BIT  & CRC 8 BIT*/
				VHT_SIG_B_generator(&pMptCtx->PMacTxInfo);

				/* 32 BIT*/
				VHT_Delimiter_generator(&pMptCtx->PMacTxInfo);
			}

			mpt_ProSetPMacTx(padapter);

		} else if (strncmp(extra, "pmact,mode=", 11) == 0) {
			int txmode = 0;

			if (sscanf(extra, "pmact,mode=%d", &txmode) > 0) {
				if (txmode == 1) {
					pMptCtx->HWTxmode = CONTINUOUS_TX;
					sprintf(extra, "\t Config HW Tx mode = CONTINUOUS_TX\n");
				} else if (txmode == 2) {
					pMptCtx->HWTxmode = OFDM_Single_Tone_TX;
					sprintf(extra, "\t Config HW Tx mode = OFDM_Single_Tone_TX\n");
				} else {
					pMptCtx->HWTxmode = PACKETS_TX;
					sprintf(extra, "\t Config HW Tx mode = PACKETS_TX\n");
				}
			} else {
				pMptCtx->HWTxmode = PACKETS_TX;
				sprintf(extra, "\t Config HW Tx mode=\n 0 = PACKETS_TX\n 1 = CONTINUOUS_TX\n 2 = OFDM_Single_Tone_TX");
			}
		} else if (strncmp(extra, "pmact,", 6) == 0) {
			int PacketPeriod = 0, PacketLength = 0, PacketCout = 0;
			int bldpc = 0, bstbc = 0;

			if (sscanf(extra, "pmact,period=%d", &PacketPeriod) > 0) {
				padapter->mppriv.pktInterval = PacketPeriod;
				RTW_INFO("PacketPeriod=%d\n", padapter->mppriv.pktInterval);
				sprintf(extra, "PacketPeriod [1~255]= %d\n", padapter->mppriv.pktInterval);

			} else if (sscanf(extra, "pmact,length=%d", &PacketLength) > 0) {
				padapter->mppriv.pktLength = PacketLength;
				RTW_INFO("PacketPeriod=%d\n", padapter->mppriv.pktLength);
				sprintf(extra, "PacketLength[~65535]=%d\n", padapter->mppriv.pktLength);

			} else if (sscanf(extra, "pmact,count=%d", &PacketCout) > 0) {
				padapter->mppriv.tx_pktcount = PacketCout;
				RTW_INFO("Packet Cout =%d\n", padapter->mppriv.tx_pktcount);
				sprintf(extra, "Packet Cout =%d\n", padapter->mppriv.tx_pktcount);

			} else if (sscanf(extra, "pmact,ldpc=%d", &bldpc) > 0) {
				pMptCtx->bldpc = bldpc;
				RTW_INFO("Set LDPC =%d\n", pMptCtx->bldpc);
				sprintf(extra, "Set LDPC =%d\n", pMptCtx->bldpc);

			} else if (sscanf(extra, "pmact,stbc=%d", &bstbc) > 0) {
				pMptCtx->bstbc = bstbc;
				RTW_INFO("Set STBC =%d\n", pMptCtx->bstbc);
				sprintf(extra, "Set STBC =%d\n", pMptCtx->bstbc);
			} else
				sprintf(extra, "\n period={1~255}\n length={1000~65535}\n count={0~}\n ldpc={0/1}\n stbc={0/1}");

		}

		wrqu->data.length = strlen(extra);
		return 0;
#endif
	} else {

		if (sscanf(extra, "ch=%d,bw=%d,rate=%d,pwr=%d,ant=%d,tx=%d", &channel, &bandwidth, &rate, &txpower, &ant, &txmode) < 6) {
			RTW_INFO("Invalid format [ch=%d,bw=%d,rate=%d,pwr=%d,ant=%d,tx=%d]\n", channel, bandwidth, rate, txpower, ant, txmode);
			memset(extra, 0, wrqu->data.length);
			sprintf(extra, "\n Please input correct format as bleow:\n");
			sprintf(extra + strlen(extra), "\t ch=%d,bw=%d,rate=%d,pwr=%d,ant=%d,tx=%d\n", channel, bandwidth, rate, txpower, ant, txmode);
			sprintf(extra + strlen(extra), "\n [ ch : BGN = <1~14> , A or AC = <36~165> ]");
			sprintf(extra + strlen(extra), "\n [ bw : Bandwidth: 0 = 20M, 1 = 40M, 2 = 80M ]");
			sprintf(extra + strlen(extra), "\n [ rate :	CCK: 1 2 5.5 11M X 2 = < 2 4 11 22 >]");
			sprintf(extra + strlen(extra), "\n [		OFDM: 6 9 12 18 24 36 48 54M X 2 = < 12 18 24 36 48 72 96 108>");
			sprintf(extra + strlen(extra), "\n [		HT 1S2SS MCS0 ~ MCS15 : < [MCS0]=128 ~ [MCS7]=135 ~ [MCS15]=143 >");
			sprintf(extra + strlen(extra), "\n [		HT 3SS MCS16 ~ MCS32 : < [MCS16]=144 ~ [MCS23]=151 ~ [MCS32]=159 >");
			sprintf(extra + strlen(extra), "\n [		VHT 1SS MCS0 ~ MCS9 : < [MCS0]=160 ~ [MCS9]=169 >");
			sprintf(extra + strlen(extra), "\n [ txpower : 1~63 power index");
			sprintf(extra + strlen(extra), "\n [ ant : <A = 1, B = 2, C = 4, D = 8> ,2T ex: AB=3 BC=6 CD=12");
			sprintf(extra + strlen(extra), "\n [ txmode : < 0 = CONTINUOUS_TX, 1 = PACKET_TX, 2 = SINGLE_TONE_TX, 3 = CARRIER_SUPPRISSION_TX, 4 = SINGLE_CARRIER_TX>\n");
			wrqu->data.length = strlen(extra);
			return status;

		} else {
			RTW_INFO("Got format [ch=%d,bw=%d,rate=%d,pwr=%d,ant=%d,tx=%d]\n", channel, bandwidth, rate, txpower, ant, txmode);
			memset(extra, 0, wrqu->data.length);
			sprintf(extra, "Change Current channel %d to channel %d", padapter->mppriv.channel , channel);
			padapter->mppriv.channel = channel;
			SetChannel(padapter);
			pHalData->current_channel = channel;

			if (bandwidth == 1)
				bandwidth = CHANNEL_WIDTH_40;
			else if (bandwidth == 2)
				bandwidth = CHANNEL_WIDTH_80;
			sprintf(extra + strlen(extra), "\nChange Current Bandwidth %d to Bandwidth %d", padapter->mppriv.bandwidth , bandwidth);
			padapter->mppriv.bandwidth = (u8)bandwidth;
			padapter->mppriv.preamble = sg;
			SetBandwidth(padapter);
			pHalData->current_channel_bw = bandwidth;

			sprintf(extra + strlen(extra), "\nSet power level :%d", txpower);
			padapter->mppriv.txpoweridx = (u8)txpower;
			pMptCtx->TxPwrLevel[ODM_RF_PATH_A] = (u8)txpower;
			pMptCtx->TxPwrLevel[ODM_RF_PATH_B] = (u8)txpower;
			pMptCtx->TxPwrLevel[ODM_RF_PATH_C] = (u8)txpower;
			pMptCtx->TxPwrLevel[ODM_RF_PATH_D]  = (u8)txpower;

			RTW_INFO("%s: bw=%d sg=%d\n", __func__, bandwidth, sg);

			if (rate <= 0x7f)
				rate = wifirate2_ratetbl_inx((u8)rate);
			else if (rate < 0xC8)
				rate = (rate - 0x80 + MPT_RATE_MCS0);
			/*HT  rate 0x80(MCS0)  ~ 0x8F(MCS15) ~ 0x9F(MCS31) 128~159
			VHT1SS~2SS rate 0xA0 (VHT1SS_MCS0 44) ~ 0xB3 (VHT2SS_MCS9 #63) 160~179
			VHT rate 0xB4 (VHT3SS_MCS0 64) ~ 0xC7 (VHT2SS_MCS9 #83) 180~199
			else
			VHT rate 0x90(VHT1SS_MCS0) ~ 0x99(VHT1SS_MCS9) 144~153
			rate =(rate - MPT_RATE_VHT1SS_MCS0);
			*/
			RTW_INFO("%s: rate index=%d\n", __func__, rate);
			if (rate >= MPT_RATE_LAST)
				return -EINVAL;
			sprintf(extra + strlen(extra), "\nSet data rate to %d index %d", padapter->mppriv.rateidx, rate);

			padapter->mppriv.rateidx = rate;
			pMptCtx->mpt_rate_index = rate;
			SetDataRate(padapter);

			sprintf(extra + strlen(extra), "\nSet Antenna Path :%d", ant);
			switch (ant) {
			case 1:
				antenna = ANTENNA_A;
				break;
			case 2:
				antenna = ANTENNA_B;
				break;
			case 4:
				antenna = ANTENNA_C;
				break;
			case 8:
				antenna = ANTENNA_D;
				break;
			case 3:
				antenna = ANTENNA_AB;
				break;
			case 5:
				antenna = ANTENNA_AC;
				break;
			case 9:
				antenna = ANTENNA_AD;
				break;
			case 6:
				antenna = ANTENNA_BC;
				break;
			case 10:
				antenna = ANTENNA_BD;
				break;
			case 12:
				antenna = ANTENNA_CD;
				break;
			case 7:
				antenna = ANTENNA_ABC;
				break;
			case 14:
				antenna = ANTENNA_BCD;
				break;
			case 11:
				antenna = ANTENNA_ABD;
				break;
			case 15:
				antenna = ANTENNA_ABCD;
				break;
			}
			RTW_INFO("%s: antenna=0x%x\n", __func__, antenna);
			padapter->mppriv.antenna_tx = antenna;
			padapter->mppriv.antenna_rx = antenna;
			pHalData->antenna_tx_path = antenna;
			SetAntenna(padapter);

			if (txmode == 0)
				pmp_priv->mode = MP_CONTINUOUS_TX;
			else if (txmode == 1) {
				pmp_priv->mode = MP_PACKET_TX;
				pmp_priv->tx.count = count;
			} else if (txmode == 2)
				pmp_priv->mode = MP_SINGLE_TONE_TX;
			else if (txmode == 3)
				pmp_priv->mode = MP_CARRIER_SUPPRISSION_TX;
			else if (txmode == 4)
				pmp_priv->mode = MP_SINGLE_CARRIER_TX;

			status = rtw_mp_pretx_proc(padapter, bStartTest, extra);
		}

	}

	wrqu->data.length = strlen(extra);
	return status;
}


int rtw_mp_rx(struct net_device *dev,
	      struct iw_request_info *info,
	      union iwreq_data *wrqu, char *extra)
{
	PADAPTER padapter = rtw_netdev_priv(dev);
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(padapter);
	struct mp_priv *pmp_priv = &padapter->mppriv;
	PMPT_CONTEXT		pMptCtx = &(padapter->mppriv.mpt_ctx);

	u32 bandwidth = 0, sg = 0, channel = 6, ant = 0;
	u16 antenna = 0;
	u8 bStartRx = 0;

	if (copy_from_user(extra, wrqu->data.pointer, wrqu->data.length))
		return -EFAULT;

#ifdef CONFIG_CONCURRENT_MODE
	if (!is_primary_adapter(padapter)) {
		sprintf(extra, "Error: MP mode can't support Virtual Adapter, Please to use main Adapter.\n");
		wrqu->data.length = strlen(extra);
		return 0;
	}
#endif

	if (strncmp(extra, "stop", 4) == 0) {
		memset(extra, 0, wrqu->data.length);
		SetPacketRx(padapter, bStartRx, false);
		pmp_priv->bmac_filter = false;
		sprintf(extra, "Received packet OK:%d CRC error:%d ,Filter out:%d", padapter->mppriv.rx_pktcount, padapter->mppriv.rx_crcerrpktcount, padapter->mppriv.rx_pktcount_filter_out);
		wrqu->data.length = strlen(extra);
		return 0;

	} else if (sscanf(extra, "ch=%d,bw=%d,ant=%d", &channel, &bandwidth, &ant) < 3) {
		RTW_INFO("Invalid format [ch=%d,bw=%d,ant=%d]\n", channel, bandwidth, ant);
		memset(extra, 0, wrqu->data.length);
		sprintf(extra, "\n Please input correct format as bleow:\n");
		sprintf(extra + strlen(extra), "\t ch=%d,bw=%d,ant=%d\n", channel, bandwidth, ant);
		sprintf(extra + strlen(extra), "\n [ ch : BGN = <1~14> , A or AC = <36~165> ]");
		sprintf(extra + strlen(extra), "\n [ bw : Bandwidth: 0 = 20M, 1 = 40M, 2 = 80M ]");
		sprintf(extra + strlen(extra), "\n [ ant : <A = 1, B = 2, C = 4, D = 8> ,2T ex: AB=3 BC=6 CD=12");
		wrqu->data.length = strlen(extra);
		return 0;

	} else {
		bStartRx = 1;
		RTW_INFO("Got format [ch=%d,bw=%d,ant=%d]\n", channel, bandwidth, ant);
		memset(extra, 0, wrqu->data.length);
		sprintf(extra, "Change Current channel %d to channel %d", padapter->mppriv.channel , channel);
		padapter->mppriv.channel = channel;
		SetChannel(padapter);
		pHalData->current_channel = channel;

		if (bandwidth == 1)
			bandwidth = CHANNEL_WIDTH_40;
		else if (bandwidth == 2)
			bandwidth = CHANNEL_WIDTH_80;
		sprintf(extra + strlen(extra), "\nChange Current Bandwidth %d to Bandwidth %d", padapter->mppriv.bandwidth , bandwidth);
		padapter->mppriv.bandwidth = (u8)bandwidth;
		padapter->mppriv.preamble = sg;
		SetBandwidth(padapter);
		pHalData->current_channel_bw = bandwidth;

		sprintf(extra + strlen(extra), "\nSet Antenna Path :%d", ant);
		switch (ant) {
		case 1:
			antenna = ANTENNA_A;
			break;
		case 2:
			antenna = ANTENNA_B;
			break;
		case 4:
			antenna = ANTENNA_C;
			break;
		case 8:
			antenna = ANTENNA_D;
			break;
		case 3:
			antenna = ANTENNA_AB;
			break;
		case 5:
			antenna = ANTENNA_AC;
			break;
		case 9:
			antenna = ANTENNA_AD;
			break;
		case 6:
			antenna = ANTENNA_BC;
			break;
		case 10:
			antenna = ANTENNA_BD;
			break;
		case 12:
			antenna = ANTENNA_CD;
			break;
		case 7:
			antenna = ANTENNA_ABC;
			break;
		case 14:
			antenna = ANTENNA_BCD;
			break;
		case 11:
			antenna = ANTENNA_ABD;
			break;
		case 15:
			antenna = ANTENNA_ABCD;
			break;
		}
		RTW_INFO("%s: antenna=0x%x\n", __func__, antenna);
		padapter->mppriv.antenna_tx = antenna;
		padapter->mppriv.antenna_rx = antenna;
		pHalData->antenna_tx_path = antenna;
		SetAntenna(padapter);

		sprintf(extra + strlen(extra), "\nstart Rx");
		SetPacketRx(padapter, bStartRx, false);
	}
	wrqu->data.length = strlen(extra);
	return 0;
}


int rtw_mp_hwtx(struct net_device *dev,
		struct iw_request_info *info,
		union iwreq_data *wrqu, char *extra)
{
	PADAPTER padapter = rtw_netdev_priv(dev);
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(padapter);
	struct mp_priv *pmp_priv = &padapter->mppriv;
	PMPT_CONTEXT		pMptCtx = &(padapter->mppriv.mpt_ctx);

	return 0;
}

int rtw_efuse_mask_file(struct net_device *dev,
			struct iw_request_info *info,
			union iwreq_data *wrqu, char *extra)
{
	char *rtw_efuse_mask_file_path;
	u8 Status;
	PADAPTER padapter = rtw_netdev_priv(dev);

	memset(maskfileBuffer, 0x00, sizeof(maskfileBuffer));

	if (copy_from_user(extra, wrqu->data.pointer, wrqu->data.length))
		return -EFAULT;

	if (strncmp(extra, "off", 3) == 0 && strlen(extra) < 4) {
		padapter->registrypriv.boffefusemask = 1;
		sprintf(extra, "Turn off Efuse Mask\n");
		wrqu->data.length = strlen(extra);
		return 0;
	}
	if (strncmp(extra, "on", 2) == 0 && strlen(extra) < 3) {
		padapter->registrypriv.boffefusemask = 0;
		sprintf(extra, "Turn on Efuse Mask\n");
		wrqu->data.length = strlen(extra);
		return 0;
	}
	if (strncmp(extra, "data,", 5) == 0) {
		u8	*pch, *pdata;
		char	*ptmp, tmp;
		u8	count = 0;
		u8	i = 0;
		u32	datalen = 0;

		ptmp = extra;
		pch = strsep(&ptmp, ",");

		if ((pch == NULL) || (strlen(pch) == 0)) {
			RTW_INFO("%s: parameter error(no cmd)!\n", __func__);
			return -EFAULT;
		}

		do {
			pch = strsep(&ptmp, ":");
			if ((pch == NULL) || (strlen(pch) == 0))
				break;
			if (strlen(pch) != 2
				|| IsHexDigit(*pch) == false
				|| IsHexDigit(*(pch + 1)) == false
				|| sscanf(pch, "%hhx", &tmp) != 1
			) {
				RTW_INFO("%s: invalid 8-bit hex! input format: data,01:23:45:67:89:ab:cd:ef...\n", __func__);
				return -EFAULT;
			}
			maskfileBuffer[count++] = tmp;

		 } while (count < 64);

		for (i = 0; i < count; i++)
			sprintf(extra + strlen(extra), ":%02x", maskfileBuffer[i]);

		padapter->registrypriv.bFileMaskEfuse = true;

		sprintf(extra + strlen(extra), "\nLoad Efuse Mask data %d hex ok\n", count);
		wrqu->data.length = strlen(extra);
		return 0;
	}
	rtw_efuse_mask_file_path = extra;

	if (rtw_is_file_readable(rtw_efuse_mask_file_path) == true) {
		RTW_INFO("%s do rtw_efuse_mask_file_read = %s! ,sizeof maskfileBuffer %zu\n", __func__, rtw_efuse_mask_file_path, sizeof(maskfileBuffer));
		Status = rtw_efuse_file_read(padapter, rtw_efuse_mask_file_path, maskfileBuffer, sizeof(maskfileBuffer));
		if (Status == true)
			padapter->registrypriv.bFileMaskEfuse = true;
		sprintf(extra, "efuse mask file read OK\n");
	} else {
		padapter->registrypriv.bFileMaskEfuse = false;
		sprintf(extra, "efuse mask file readable FAIL\n");
		RTW_INFO("%s rtw_is_file_readable fail!\n", __func__);
	}
	wrqu->data.length = strlen(extra);
	return 0;
}


int rtw_efuse_file_map(struct net_device *dev,
		       struct iw_request_info *info,
		       union iwreq_data *wrqu, char *extra)
{
	char *rtw_efuse_file_map_path;
	u8 Status;
	PEFUSE_HAL pEfuseHal;
	PADAPTER padapter = rtw_netdev_priv(dev);
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	struct mp_priv *pmp_priv = &padapter->mppriv;

	pEfuseHal = &pHalData->EfuseHal;
	if (copy_from_user(extra, wrqu->data.pointer, wrqu->data.length))
		return -EFAULT;

	rtw_efuse_file_map_path = extra;

	memset(pEfuseHal->fakeEfuseModifiedMap, 0xFF, EFUSE_MAX_MAP_LEN);

	if (rtw_is_file_readable(rtw_efuse_file_map_path) == true) {
		RTW_INFO("%s do rtw_efuse_mask_file_read = %s!\n", __func__, rtw_efuse_file_map_path);
		Status = rtw_efuse_file_read(padapter, rtw_efuse_file_map_path, pEfuseHal->fakeEfuseModifiedMap, sizeof(pEfuseHal->fakeEfuseModifiedMap));
		if (Status == true) {
			pmp_priv->bloadefusemap = true;
			sprintf(extra, "efuse file file_read OK\n");
		} else {
			pmp_priv->bloadefusemap = false;
			sprintf(extra, "efuse file file_read FAIL\n");
		}
	} else {
		sprintf(extra, "efuse file readable FAIL\n");
		RTW_INFO("%s rtw_is_file_readable fail!\n", __func__);
	}
	wrqu->data.length = strlen(extra);
	return 0;
}

#endif
