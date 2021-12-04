#ifndef _EVHTTPCLIENT_H
#define _EVHTTPCLIENT_H

#include <event2/event.h>
#include <event2/http.h>
#include <event2/http_struct.h>
#include <functional>
#include <map>
#include <utility>
#include <event2/buffer.h>

using http_header_t = std::map<std::string, std::string>;
class HTTPRequest
{
public:
    HTTPRequest()
    {
        m_header["Content-Type"] = "application/json;charset=UTF-8";
    }

    HTTPRequest& AddHeader(const std::string& key, const std::string& value)
    {
        m_header[key] = value;
        return *this;
    }

    const http_header_t& GetHeader()
    {
        return m_header;
    }

    HTTPRequest& SetParam(const std::string& param)
    {
        m_param = param;
        return *this;
    }

    const std::string& GetParam()
    {
        return m_param;
    }

    HTTPRequest& SetTimeout(int32_t sec)
    {
        m_sec = sec;
        return *this;
    }

    int32_t GetTimeout() const
    {
        return m_sec;
    }

    HTTPRequest& SetPostData(const std::string& post)
    {
        m_post_data = post;
        return *this;
    }

    const std::string& GetPostData()
    {
        return m_post_data;
    }

    HTTPRequest& SetHandler(std::function<void(const std::string&)> handler)
    {
        m_handler = std::move(handler);
        return *this;
    }

    HTTPRequest& SetRemote(const std::string& host, int32_t port)
    {
        m_host = host;
        m_port = port;
        return *this;
    }

    const std::string& GetHost()
    {
        return m_host;
    }

    int32_t GetPort() const
    {
        return m_port;
    }

    void Handle(const std::string& data)
    {
        if (m_handler)
        {
            m_handler(data);
        }
    }

    void SetMethod(evhttp_cmd_type method)
    {
        m_method = method;
    }

    evhttp_cmd_type GetMethod()
    {
        return m_method;
    }

    void SetCloseOnComplete(bool is_close_on_complete)
    {
        m_is_close_on_complete = is_close_on_complete;
    }

    bool IsCloseOnComplete() const
    {
        return m_is_close_on_complete;
    }
private:
    int32_t m_sec = 0;
    int32_t m_port = 0;
    std::string m_host;
    std::string m_param;
    std::string m_post_data;
    http_header_t m_header;
    bool m_is_close_on_complete = true;
    evhttp_cmd_type m_method = EVHTTP_REQ_POST;
    std::function<void(const std::string&)> m_handler;
};

class HTTP
{
private:
    struct HttpRequestCtx
    {
        bool m_has_error = false;
        HTTPRequest* ctx = nullptr;
    };

    static void OnResponse(struct evhttp_request * remote_rsp, void *arg)
    {
        auto* ctx = static_cast<HttpRequestCtx*>(arg);
        if (remote_rsp)
        {
            struct evbuffer* buffer = evhttp_request_get_input_buffer(remote_rsp);
            size_t size = evbuffer_get_length(buffer);
            if (size)
            {
                std::string data(size, '\0');
                evbuffer_remove(buffer, &data.front(), size);
                ctx->ctx->Handle(data);
            }
        }
        delete ctx->ctx;
        delete ctx;
    }

    static void OnError(evhttp_request_error error, void * arg)
    {
        auto* ctx = static_cast<HttpRequestCtx*>(arg);
        ctx->m_has_error = true;
    }

public:
    static bool MakeRequest(event_base* base, HTTPRequest*& request)
    {
        evhttp_connection* con = evhttp_connection_base_new(base, nullptr, request->GetHost().c_str(), request->GetPort());
        if (!con)
        {
            return false;
        }
        auto* request_ctx = new HttpRequestCtx();
        request_ctx->ctx = request;
        evhttp_connection_set_timeout(con, request->GetTimeout());
        evhttp_connection_free_on_completion(con);

        auto* req = evhttp_request_new(OnResponse, request_ctx);
        evhttp_request_set_error_cb(req, OnError);

        auto http_header = evhttp_request_get_output_headers(req);
        auto& header_table = request->GetHeader();
        for (auto& header : header_table)
        {
            evhttp_add_header(http_header, header.first.c_str(), header.second.c_str());
        }
        std::string hp = request->GetHost() + ":" + std::to_string(request->GetPort());
        evhttp_add_header(http_header, "Host", hp.c_str());
        if (request->IsCloseOnComplete())
        {
            // https://github.com/libevent/libevent/issues/115
            evhttp_add_header(http_header, "Connection", "close");
        }
        if (!request->GetPostData().empty())
        {
            evbuffer_add(req->output_buffer, request->GetPostData().c_str(), request->GetPostData().size());
        }
        evhttp_make_request(con, req, request->GetMethod(), request->GetParam().c_str());
        if (request_ctx->m_has_error)
        {
            evhttp_connection_free(con);
            return false;
        }
        request = nullptr;
        return true;
    }
};

#endif //_EVHTTPCLIENT_H
