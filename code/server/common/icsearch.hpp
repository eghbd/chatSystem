#pragma once
#include <elasticlient/client.h>   // ES 客户端（封装 ES 的 HTTP 接口）
#include <cpr/cpr.h>               // HTTP 请求库（elasticlient 底层用它发请求）
#include <json/json.h>             // jsoncpp，负责 Json::Value 与字符串互转
#include <iostream>
#include <memory>
#include "logger.hpp"              // 日志宏 LOG_ERROR / LOG_DEBUG

namespace bite_im {

/* ============================================================
 * 一、JSON 序列化 / 反序列化工具
 *    Serialize  : Json::Value -> string
 *    UnSerialize: string -> Json::Value
 * ============================================================ */

// 序列化：把 Json::Value 转成 JSON 字符串
bool Serialize(const Json::Value &val, std::string &dst) {
    // StreamWriterBuilder：创建 Json 写入器的工厂
    Json::StreamWriterBuilder swb;
    swb.settings_["emitUTF8"] = true;   // 中文按 UTF-8 原样输出，不转成 \uXXXX
    // newStreamWriter 返回裸指针，用 unique_ptr 托管自动释放
    std::unique_ptr<Json::StreamWriter> sw(swb.newStreamWriter());

    // write 把 val 写入流，返回 0 表示成功
    std::stringstream ss;
    int ret = sw->write(val, &ss);
    if (ret != 0) {
        std::cout << "Json反序列化失败！\n";   // ⚠️ 文案笔误：此处是序列化
        return false;
    }
    dst = ss.str();
    return true;
}

// 反序列化：把 JSON 字符串解析成 Json::Value
bool UnSerialize(const std::string &src, Json::Value &val) {
    Json::CharReaderBuilder crb;        // 创建 Json 读取器的工厂
    std::unique_ptr<Json::CharReader> cr(crb.newCharReader());

    std::string err;                    // 解析失败时带回错误信息
    // 用 [begin, end) 指针区间解析，比传 c_str() 更安全（支持含 '\0' 的串）
    bool ret = cr->parse(src.c_str(), src.c_str() + src.size(), &val, &err);
    if (ret == false) {
        std::cout << "json反序列化失败: " << err << std::endl;
        return false;
    }
    return true;
}


/* ============================================================
 * 二、ESIndex —— 建索引（settings + mappings）
 *     链式 append 声明字段，create 一次性提交
 * ============================================================ */
class ESIndex {
public:
    ESIndex(std::shared_ptr<elasticlient::Client> &client,
            const std::string &name,
            const std::string &type = "_doc")
        : _name(name), _type(type), _client(client) {
        // 构造时预置 IK 中文分词器配置（写进 settings.analysis）
        Json::Value analysis;    // 最外层：analysis
        Json::Value analyzer;    // analysis.analyzer
        Json::Value ik;          // analysis.analyzer.ik
        Json::Value tokenizer;   // analysis.analyzer.ik.tokenizer
        tokenizer["tokenizer"] = "ik_max_word";  // 用 ik_max_word 分词器（最细粒度切词）
        ik["ik"] = tokenizer;
        analyzer["analyzer"] = ik;
        analysis["analysis"] = analyzer;
        _index["settings"] = analysis;   // 存进 _index.settings
    }

    // 链式声明一个字段的 mapping；返回 *this 引用以支持连续调用
    ESIndex& append(const std::string &key,
                    const std::string &type = "text",
                    const std::string &analyzer = "ik_max_word",
                    bool enabled = true) {
        Json::Value fields;
        fields["type"] = type;
        //  坑：type=keyword 时不应带 analyzer，ES 会忽略或报错
        fields["analyzer"] = analyzer;
        if (enabled == false) fields["enabled"] = enabled;  // 不索引、只存储
        _properties[key] = fields;
        return *this;   // 链式关键：返回自身引用
    }

    // 真正创建索引：组装 mappings，序列化后发请求
    bool create(const std::string &index_id = "default_index_id") {
        Json::Value mappings;
        mappings["dynamic"] = true;          // 允许动态字段（未声明的字段自动推断类型）
        mappings["properties"] = _properties;
        _index["mappings"] = mappings;

        // 1. 序列化索引定义
        std::string body;
        bool ret = Serialize(_index, body);
        if (ret == false) {
            LOG_ERROR("索引序列化失败！");
            return false;
        }
        LOG_DEBUG("{}", body);

        // 2. 发起创建请求（elasticlient 用 index 接口提交索引定义）
        try {
            auto rsp = _client->index(_name, _type, index_id, body);
            if (rsp.status_code < 200 || rsp.status_code >= 300) {
                //  索引已存在时 ES 返回 400，会被当失败，重复初始化需注意
                LOG_ERROR("创建ES索引 {} 失败，响应状态码异常: {}", _name, rsp.status_code);
                return false;
            }
        } catch (std::exception &e) {
            LOG_ERROR("创建ES索引 {} 失败: {}", _name, e.what());
            return false;
        }
        return true;
    }

private:
    std::string _name;              // 索引名
    std::string _type;              // 类型（ES7+ 统一 _doc）
    Json::Value _properties;        // 累积的字段映射
    Json::Value _index;             // 完整索引定义（settings + mappings）
    std::shared_ptr<elasticlient::Client> _client;  // ES 客户端（共享）
};


/* ============================================================
 * 三、ESInsert —— 插入 / 更新文档
 *     链式 append 攒字段，insert 提交；id 作文档 _id
 * ============================================================ */
class ESInsert {
public:
    ESInsert(std::shared_ptr<elasticlient::Client> &client,
             const std::string &name,
             const std::string &type = "_doc")
        : _name(name), _type(type), _client(client) {}

    // 模板方法：支持任意类型（string/int/bool/Json::Value），不丢 JSON 类型
    template<typename T>
    ESInsert &append(const std::string &key, const T &val) {
        _item[key] = val;   // jsoncpp 的 Value 有对应类型重载赋值
        return *this;       // 链式关键
    }

    // 提交插入；id 为空则 ES 自动生成，非空则作为文档 _id（同 ID 重复插入=覆盖）
    bool insert(const std::string id = "") {
        std::string body;
        bool ret = Serialize(_item, body);
        if (ret == false) {
            LOG_ERROR("索引序列化失败！");
            return false;
        }
        LOG_DEBUG("{}", body);

        try {
            // PUT /<name>/<type>/<id>  body 为文档内容
            auto rsp = _client->index(_name, _type, id, body);
            if (rsp.status_code < 200 || rsp.status_code >= 300) {
                LOG_ERROR("新增数据 {} 失败，响应状态码异常: {}", body, rsp.status_code);
                return false;
            }
        } catch (std::exception &e) {
            LOG_ERROR("新增数据 {} 失败: {}", body, e.what());
            return false;
        }
        return true;
    }

private:
    std::string _name;
    std::string _type;
    Json::Value _item;              // 累积的文档字段
    std::shared_ptr<elasticlient::Client> _client;
};


/* ============================================================
 * 四、ESRemove —— 按 ID 删除文档
 * ============================================================ */
class ESRemove {
public:
    ESRemove(std::shared_ptr<elasticlient::Client> &client,
             const std::string &name,
             const std::string &type = "_doc")
        : _name(name), _type(type), _client(client) {}

    bool remove(const std::string &id) {
        try {
            // DELETE /<name>/<type>/<id>
            auto rsp = _client->remove(_name, _type, id);
            if (rsp.status_code < 200 || rsp.status_code >= 300) {
                // 删除不存在的 ID，ES 返回 404，会被判失败（幂等删除应忽略）
                LOG_ERROR("删除数据 {} 失败，响应状态码异常: {}", id, rsp.status_code);
                return false;
            }
        } catch (std::exception &e) {
            LOG_ERROR("删除数据 {} 失败: {}", id, e.what());
            return false;
        }
        return true;
    }

private:
    std::string _name;
    std::string _type;
    std::shared_ptr<elasticlient::Client> _client;
};


/* ============================================================
 * 五、ESSearch —— 构造 bool 查询并搜索
 *     must_not / should / must 三组条件链式累积，search 提交
 * ============================================================ */
class ESSearch {
public:
    ESSearch(std::shared_ptr<elasticlient::Client> &client,
             const std::string &name,
             const std::string &type = "_doc")
        : _name(name), _type(type), _client(client) {}

    // 排除条件：字段值不在 vals 中（terms 不分词，适合 keyword 字段）
    ESSearch& append_must_not_terms(const std::string &key,
                                    const std::vector<std::string> &vals) {
        Json::Value fields;
        for (const auto &val : vals) {
            fields[key].append(val);   // fields[key] = ["v1","v2",...]
        }
        Json::Value terms;
        terms["terms"] = fields;       // { "terms": { "key": [..] } }
        _must_not.append(terms);
        return *this;
    }

    // 或条件：match 分词匹配（满足任一 should 即可，默认 minimum_should_match=1）
    ESSearch& append_should_match(const std::string &key, const std::string &val) {
        Json::Value field;
        field[key] = val;
        Json::Value match;
        match["match"] = field;        // { "match": { "key": val } }
        _should.append(match);
        return *this;
    }

    // 必须条件：term 精确匹配（不分词，适合 keyword 字段）
    ESSearch& append_must_term(const std::string &key, const std::string &val) {
        Json::Value field;
        field[key] = val;
        Json::Value term;
        term["term"] = field;          // { "term": { "key": val } }
        _must.append(term);
        return *this;
    }

    // 必须条件：match 分词匹配（全文检索，适合 text 字段）
    ESSearch& append_must_match(const std::string &key, const std::string &val) {
        Json::Value field;
        field[key] = val;
        Json::Value match;
        match["match"] = field;        // { "match": { "key": val } }
        _must.append(match);
        return *this;
    }

    // 执行搜索：组装 bool 查询 → 发请求 → 反序列化 → 取 hits.hits
    Json::Value search() {
        // 1. 组装 bool 查询
        Json::Value cond;
        if (_must_not.empty() == false) cond["must_not"] = _must_not;
        if (_should.empty()   == false) cond["should"]   = _should;
        if (_must.empty()     == false) cond["must"]     = _must;
        Json::Value query;
        query["bool"] = cond;
        Json::Value root;
        root["query"] = query;
        // ⚠️ 未设分页(from/size)、排序(sort)，大结果集会全量返回

        // 2. 序列化请求体
        std::string body;
        bool ret = Serialize(root, body);
        if (ret == false) {
            LOG_ERROR("索引序列化失败！");
            return Json::Value();
        }
        LOG_DEBUG("{}", body);

        // 3. 发起搜索请求
        cpr::Response rsp;
        try {
            rsp = _client->search(_name, _type, body);
            if (rsp.status_code < 200 || rsp.status_code >= 300) {
                LOG_ERROR("检索数据 {} 失败，响应状态码异常: {}", body, rsp.status_code);
                return Json::Value();
            }
        } catch (std::exception &e) {
            LOG_ERROR("检索数据 {} 失败: {}", body, e.what());
            return Json::Value();
        }

        // 4. 反序列化响应正文
        LOG_DEBUG("检索响应正文: [{}]", rsp.text);
        Json::Value json_res;
        ret = UnSerialize(rsp.text, json_res);
        if (ret == false) {
            LOG_ERROR("检索数据 {} 结果反序列化失败", rsp.text);
            return Json::Value();
        }
        // 返回命中文档数组（每项含 _id/_score/_source）
        return json_res["hits"]["hits"];
    }

private:
    std::string _name;
    std::string _type;
    Json::Value _must_not;   // 累积 must_not 条件
    Json::Value _should;     // 累积 should 条件
    Json::Value _must;       // 累积 must 条件
    std::shared_ptr<elasticlient::Client> _client;
};

} // namespace bite_im