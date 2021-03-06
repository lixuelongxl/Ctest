/*
 * =====================================================================================
 *
 *       Filename:  doc_manager.h
 *
 *    Description:  doc manager class definition.
 *
 *        Version:  1.0
 *        Created:  09/08/2018
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  zhulin, shzhulin3@jd.com
 *        Company:  JD.com, Inc.
 *
 * =====================================================================================
 */

#ifndef __DOC_MANAGER_H__
#define __DOC_MANAGER_H__
#include "comm.h"
#include "json/json.h"
#include <map>
#include <set>

class RequestContext;
struct GeoPointContext;

class DocManager{
public:
    DocManager(RequestContext *c);
    ~DocManager();

    bool CheckDocByExtraFilterKey(std::string doc_id);
    bool GetDocContent();
    bool GetDocContent(const GeoPointContext& geo_point , std::vector<IndexInfo>& index_infos);

    bool AppendFieldsToRes(Json::Value &response, std::vector<std::string> &m_fields);
    bool GetScoreMap(std::string doc_id, uint32_t m_sort_type, std::string m_sort_field, FIELDTYPE &m_sort_field_type);
    std::map<std::string, std::string>& ScoreStrMap();
    std::map<std::string, int>& ScoreIntMap();
    std::map<std::string, double>& ScoreDoubleMap();
    std::map<std::string, uint32_t>& ValidVersion();

private:
    void CheckIfKeyValid(const std::vector<ExtraFilterKey>& extra_filter_vec, const Json::Value &value, bool flag, bool &key_valid);

private:
    std::map<std::string, std::string> score_str_map;
    std::map<std::string, int> score_int_map;
    std::map<std::string, double> score_double_map;
    std::map<std::string, uint32_t> valid_version_;
    hash_string_map doc_content_map_;
    RequestContext* component;
};

#endif