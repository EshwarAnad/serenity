/*
 * Copyright (c) 2018-2020, The SerenityOS developers.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <LibHTTP/HttpRequest.h>
#include <LibHTTP/HttpsJob.h>
#include <ProtocolServer/HttpsDownload.h>
#include <ProtocolServer/HttpsProtocol.h>

namespace ProtocolServer {

HttpsProtocol::HttpsProtocol()
    : Protocol("https")
{
}

HttpsProtocol::~HttpsProtocol()
{
}

OwnPtr<Download> HttpsProtocol::start_download(ClientConnection& client, const String& method, const URL& url, const HashMap<String, String>& headers, ReadonlyBytes body)
{
    HTTP::HttpRequest request;
    if (method.equals_ignoring_case("post"))
        request.set_method(HTTP::HttpRequest::Method::POST);
    else
        request.set_method(HTTP::HttpRequest::Method::GET);
    request.set_url(url);
    request.set_headers(headers);
    request.set_body(body);

    int fd_pair[2] { 0 };
    if (pipe(fd_pair) != 0) {
        auto saved_errno = errno;
        dbgln("Protocol: pipe() failed: {}", strerror(saved_errno));
        return nullptr;
    }
    auto output_stream = make<OutputFileStream>(fd_pair[1]);
    output_stream->make_unbuffered();
    auto job = HTTP::HttpsJob::construct(request, *output_stream);
    auto download = HttpsDownload::create_with_job({}, client, (HTTP::HttpsJob&)*job, move(output_stream));
    download->set_download_fd(fd_pair[0]);
    job->start();
    return download;
}

}
