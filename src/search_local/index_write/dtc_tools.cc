/*
 * =====================================================================================
 *
 *       Filename:  dtc_tools.cc
 *
 *    Description:  DTCTools class definition.
 *
 *        Version:  1.0
 *        Created:  09/08/2020 10:02:05 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  shrewdlin, linjinming@jd.com
 *        Company:  JD.com, Inc.
 *
 * =====================================================================================
 */

#include "dtc_tools.h"
#include "split_manager.h"
#include "log.h"
#include "comm.h"
#include <iostream>
#include <sstream>
#include <string.h>

string initial_table[] = { "b","p","m","f","d","t","n","l","g","k","h","j","q","x","zh","ch","sh","r","z","c","s","y","w" };

int DTCTools::init_servers(DTC::DTCServers &servers, SDTCHost &dtc_config)
{
	int ret = 0;
	ret = servers.SetTableName(dtc_config.szTablename.c_str());
	if (0 != ret)
	{
		cout << "SetTableName error !\n";
		return ret;
	}
	std::vector<DTC::ROUTE_NODE> list;
	for(std::vector<SDTCroute>::const_iterator route_elem = dtc_config.vecRoute.begin(); route_elem != dtc_config.vecRoute.end(); route_elem++)
	{
		DTC::ROUTE_NODE route;
		route.bid = route_elem->uBid;
		route.port = route_elem->uPort;
		route.status = route_elem->uStatus;
		route.weight = route_elem->uWeight;
		memcpy(route.ip,route_elem->szIpadrr.c_str(), strlen(route_elem->szIpadrr.c_str()));
		route.ip[strlen(route_elem->szIpadrr.c_str())] = '\0';
		list.push_back(route);
	}
	ret = servers.SetRouteList(list);
	if (0 != ret) {
		cout << "SetRouteList error!\n";
		return ret;
	}

	servers.SetMTimeout(dtc_config.uTimeout);
	ret = servers.SetAccessKey(dtc_config.szAccesskey.c_str());
	if (0 != ret)
	{
		cout << "SetAccessKey error !\n";
		return ret;
	}
	ret = servers.SetKeyType(dtc_config.uKeytype);
	if (0 != ret)
	{
		cout << "SetKeyType error !\n";
		return ret;
	}
	return ret;
}

int DTCTools::init_dtc_server(DTC::Server &server, const char *ip_str, const char *dtc_port, SDTCHost &dtc_config)
{
	int ret = 0;
	ret = server.SetTableName(dtc_config.szTablename.c_str());
	if (0 != ret)
	{
		cout << "SetTableName error !\n";
		return ret;
	}
	server.SetAddress(ip_str, dtc_port);
	server.SetMTimeout(dtc_config.uTimeout);
	if(1 == dtc_config.uKeytype || 2 == dtc_config.uKeytype)
		server.IntKey();
	else
		server.StringKey();
	return ret;
}

bool DTCTools::insert_dtc_server(u_int64_t ip_port_key,const char *ip_str,const char *port_str,SDTCHost &dtc_config){
	DTC::Server s;
	init_dtc_server(s,ip_str,port_str,dtc_config);
	dtc_handle.insert(make_pair(ip_port_key,s));
	return true;
}

string gen_dtc_key_string(uint32_t appid, string type, string key) {
	stringstream ss;
	ss << appid << "#" << type << "#" << key;
	return ss.str();
}

string gen_dtc_key_string(uint32_t appid, string type, uint32_t key) {
	stringstream ss;
	ss << appid << "#" << type << "#" << key;
	return ss.str();
}

string gen_dtc_key_string(uint32_t appid, string type, int64_t key) {
	stringstream ss;
	ss << appid << "#" << type << "#" << key;
	return ss.str();
}

string gen_dtc_key_string(uint32_t appid, string type, double key) {
	stringstream ss;
	ss << appid << "#" << type << "#" << key;
	return ss.str();
}

void split_func(string pinyin, string &split_str) {
	int i = 0;
	stringstream result;
	for (i = 0; i < (int)pinyin.size(); i++)
	{
		if (strchr("aeiouv", pinyin.at(i)))
		{
			result << pinyin.at(i);
			continue;
		}
		else
		{
			if (pinyin.at(i) != 'n')  //??????n?????????????????????
			{
				if (i == 0)
				{
					result << pinyin.at(i);
				}
				else
				{
					result << ' ' << pinyin.at(i);
				}
				if ((i + 1) < (int)pinyin.size() && (pinyin.at(i) == 'z' || pinyin.at(i) == 'c' || pinyin.at(i) == 's') &&
					(pinyin.at(i + 1) == 'h'))
				{
					result << 'h';
					i++;
				}
				continue;
			}
			else                 //???n,????????????
			{
				if (i == (int)pinyin.size() - 1)
				{
					result << pinyin.at(i);
					continue;
				}
				else
					i++;   //????????????

				if (strchr("aeiouv", pinyin.at(i)))   //???????????????,???n?????????
				{
					if (i == 1)
					{
						result << 'n' << pinyin.at(i);
						continue;
					}
					else
					{
						result << ' ' << 'n' << pinyin.at(i);
						continue;
					}
				}
				//?????????????????????
				else
				{
					if (pinyin.at(i) == 'g')
					{
						if (i == (int)pinyin.size() - 1)
						{
							result << 'n' << pinyin.at(i);
							continue;
						}
						else
							i++;  //????????????

						if (strchr("aeiouv", pinyin.at(i)))
						{
							result << 'n' << ' ' << 'g' << pinyin.at(i);
							continue;
						}
						else
						{
							result << 'n' << 'g' << ' ' << pinyin.at(i);
							if ((i + 1) < (int)pinyin.size() && (pinyin.at(i) == 'z' || pinyin.at(i) == 'c' || pinyin.at(i) == 's') &&
								(pinyin.at(i + 1) == 'h'))
							{
								result << 'h';
								i++;
							}
							continue;
						}
					}
					else   //??????g???????????????,???n?????????
					{
						result << 'n' << ' ' << pinyin.at(i);
						if ((i + 1) < (int)pinyin.size() && (pinyin.at(i) == 'z' || pinyin.at(i) == 'c' || pinyin.at(i) == 's') &&
							(pinyin.at(i + 1) == 'h'))
						{
							result << 'h';
							i++;
						}
						continue;
					}
				}
			}
		}
	}
	split_str = result.str();
}

void convert_intelligent_alpha_num(const vector<Content> &result, vector<IntelligentInfo> &info_vec, bool &flag) {
	int i = 0;
	flag = true;
	IntelligentInfo basic_info;
	vector<Content>::const_iterator content_iter = result.begin();
	for (; content_iter != result.end(); content_iter++, i++) {
		if (i >= 16) {
			log_info("content length[%d] must be less than 16", (int)result.size());
			break;
		}
		basic_info.initial_char[i] = ((*content_iter).str)[0];
	}
	info_vec.push_back(basic_info);
}

void convert_intelligent(const vector<Content> &result, vector<IntelligentInfo> &info_vec, bool &flag) {
	int i = 0;
	flag = true;
	IntelligentInfo basic_info;
	vector<vector<string> > phonetic_id_vecs;
	vector<uint32_t> length_vec;
	vector<Content>::const_iterator content_iter = result.begin();
	for (; content_iter != result.end(); content_iter++, i++) {
		if (i >= 8) {
			log_info("content length[%d] must be less than 8", (int)result.size());
			break;
		}
		uint32_t charact_id = 0;
		uint32_t phonetic_id = 0;
		vector<string> phonetic_id_vec;
		if ((*content_iter).type == CHINESE) { // ?????????id
			SplitManager::Instance()->GetCharactId((*content_iter).str, charact_id);
			basic_info.charact_id[i] = charact_id;
			vector<string> vec = SplitManager::Instance()->GetPhonetic((*content_iter).str);
			if (vec.size() == 1) {
				phonetic_id_vec.push_back(vec[0]);
			}
			else if (vec.size() > 1) {  // ?????????
				int j = 0;
				for (; j < (int)vec.size(); j++) {
					SplitManager::Instance()->GetPhoneticId(vec[j], phonetic_id);
					phonetic_id_vec.push_back(vec[j]);
				}
			}
			phonetic_id_vecs.push_back(phonetic_id_vec);
			length_vec.push_back(phonetic_id_vec.size());
		}
		else {
			basic_info.initial_char[i] = (*content_iter).str[0];
		}
	}

	/*
	* ?????????????????????????????????????????????????????????????????????
	* ????????? ?????? ???????????????4?????????
	* chongchuan chongzhuan zhongchuan zhongzhuan
	* ??????????????????????????????????????????????????????????????????????????????????????????????????????
	* ?????????
	* int factor[3][4] =
	*	{
	*		{0, 1, 2, 3}, 
	*		{0, 1}, 
	*		{0, 1, 2}, 
	*	};
	* ?????????3x2x4???24??????????????????[0-2] [0-1] [0-3]?????????????????????????????????????????????????????????
	* ??????????????????????????????????????????????????????????????????0 - 23 ??????????????????????????????????????????
	* ????????????????????????????????????????????????????????????????????????????????????
	* ???0 - 23???24???????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????? 
	*/
	i = 0;
	int j = 0;
	int k = 0;
	int len = 0;
	int len_num = 0;
	int totalLength = 1;
	uint32_t phonetic_id = 0;
	int colum = phonetic_id_vecs.size();
	for (i = 0; i < colum; i++)
	{
		totalLength *= length_vec[i];
	}
	for (i = 0; i < totalLength; i++) {
		k = i;
		len_num = 0;
		IntelligentInfo info = basic_info;
		for (j = 0; j < colum; j++) {
			len = length_vec[len_num];
			string phonetic = phonetic_id_vecs[j][k % len];
			SplitManager::Instance()->GetPhoneticId(phonetic, phonetic_id);
			info.phonetic_id[j] = phonetic_id;
			if (phonetic.size() > 1) {
				info.initial_char[j] = phonetic[0];
			}
			k = k / len;
			len_num++;
		}
		info_vec.push_back(info);
	}
	if (info_vec.size() == 0 && phonetic_id_vecs.size() == 0) {
		info_vec.push_back(basic_info);
	}
}

void get_intelligent(string str, vector<IntelligentInfo> &info_vec, bool &flag) {
	vector<Content> result;
	set<string> initial_vec(initial_table, initial_table + 23);
	iutf8string utf8_str(str);
	int i = 0;
	if (noChinese(str)) {
		for (; i < (int)str.length(); i++) {
			Content content;
			content.str = str[i];
			content.type = INITIAL;
			result.push_back(content);
		}
		convert_intelligent_alpha_num(result, info_vec, flag);
	}
	else{
		for (; i < utf8_str.length(); ) {
			if (utf8_str[i].size() > 1) {
				Content content;
				content.type = CHINESE;
				content.str = utf8_str[i];
				result.push_back(content);
				i++;
			}
			else {
				Content content;
				content.type = INITIAL;
				content.str = utf8_str[i];
				result.push_back(content);
				i++;
			}
		}
		convert_intelligent(result, info_vec, flag);
	}
}

bool noChinese(string str) {
	iutf8string utf8_str(str);
	if (utf8_str.length() == (int)str.length()) {
		return true;
	}
	else {
		return false;
	}
}

bool allChinese(string str) {
	iutf8string utf8_str(str);
	for (int i = 0; i < utf8_str.length(); i++) {
		if (utf8_str[i].length() == 1) {
			return false;
		}
	}
	return true;
}

/*
** ????????????????????????vector?????????vector??????????????????????????????
** ?????????[[a],[b1,b2],[c1,c2,c3]]
** ?????????[a_b1_c1,a_b1_c2,a_b1_c3,a_b2_c1,a_b2_c2,a_b2_c3]
*/
vector<string> combination(vector<vector<string> > &dimensionalArr){
	int FLength = dimensionalArr.size();
	if(FLength >= 2){
		int SLength1 = dimensionalArr[0].size();
		int SLength2 = dimensionalArr[1].size();
		int DLength = SLength1 * SLength2;
		vector<string> temporary(DLength);
		int index = 0;
		for(int i = 0; i < SLength1; i++){
			for (int j = 0; j < SLength2; j++) {
				temporary[index] = dimensionalArr[0][i] +"_"+ dimensionalArr[1][j];
				index++;
			}
		}
		vector<vector<string> > new_arr;
		new_arr.push_back(temporary);
		for(int i = 2; i < (int)dimensionalArr.size(); i++){
			new_arr.push_back(dimensionalArr[i]);
		}
		return combination(new_arr);
	} else {
		return dimensionalArr[0];
	}
}

vector<int> splitInt(const string& src, string separate_character)
{
	vector<int> strs;

	//????????????????????????,???????????????????????????,,???????????????????????????
	int separate_characterLen = separate_character.size();
	int lastPosition = 0, index = -1;
	string str;
	int pos = 0;
	while (-1 != (index = src.find(separate_character, lastPosition)))
	{
		if (src.substr(lastPosition, index - lastPosition) != " ") {
			str = src.substr(lastPosition, index - lastPosition);
			pos = atoi(str.c_str());
			strs.push_back(pos);
		}
		lastPosition = index + separate_characterLen;
	}
	string lastString = src.substr(lastPosition);//???????????????????????????????????????
	if (!lastString.empty() && lastString != " "){
		pos = atoi(lastString.c_str());
		strs.push_back(pos);//???????????????????????????????????????????????????
	}
	return strs;
}