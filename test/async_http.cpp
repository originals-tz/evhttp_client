#include <iostream>
#include "../evhttpclient.h"
int main()
{
    event_base* base = event_base_new();
    auto* request = new HTTPRequest();
    request->SetRemote("127.0.0.1", 8000)
            .SetPostData("Hello")
            .SetParam("/test")
            .SetHandler([](const std::string& data) {
                std::cout << data << std::endl;
            });
    HTTP::MakeRequest(base, request);
    event_base_dispatch(base);
    event_base_free(base);

    return 0;
}
