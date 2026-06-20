// Copyright (c) 2019 Cloudflare, Inc. and contributors
// Licensed under the MIT License:
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once
// Bridges from KJ HTTP to Zap HTTP-over-RPC.

#include <zap/compat/http-over-zap.zap.h>
#include <kj/compat/http.h>
#include <kj/map.h>
#include "byte-stream.h"

ZAP_BEGIN_HEADER

namespace zap {

class HttpOverZapFactory {
public:
  class HeaderIdBundle {
  public:
    HeaderIdBundle(kj::HttpHeaderTable::Builder& builder);

    HeaderIdBundle clone() const;

  private:
    HeaderIdBundle(const kj::HttpHeaderTable& table, kj::Array<kj::HttpHeaderId> nameZapToKj,
        size_t maxHeaderId);
    // Constructor for clone().

    const kj::HttpHeaderTable& table;

    kj::Array<kj::HttpHeaderId> nameZapToKj;
    size_t maxHeaderId = 0;

    friend class HttpOverZapFactory;
  };

  enum OptimizationLevel {
    // Specifies the protocol optimization level supported by the remote peer. Setting this higher
    // will improve efficiency but breaks compatibility with older peers that don't implement newer
    // levels.

    // There used to be a LEVEL_1, which used `startRequest()`, the original version of the
    // protocol. Support for this level was removed in the v2 branch in order to simplify the code.
    // If you have existing servers in the wild implementing this protocol that don't support
    // LEVEL_2, then your clients will have to stick to Zap 1.x until those servers are all
    // updated.

    LEVEL_2
    // Use request(). This is more efficient than startRequest() but won't work with old peers that
    // only implement startRequest().
  };

  HttpOverZapFactory(ByteStreamFactory& streamFactory, HeaderIdBundle headerIds,
                       OptimizationLevel peerOptimizationLevel);
  // Note: `peerOptimizationLevel` use to be optional, but defaulted to LEVEL_1. However, any
  // client still setting this to LEVEL_1 will be unable to talk to any server who is running new
  // code where LEVEL_1 was removed. So if you hit a compile error because your code is not setting
  // this option, you will need to roll back to an older version of Zap for now, until you
  // can update all code in production to pass LEVEL_2 here.

  kj::Own<kj::HttpService> zapToKj(zap::HttpService::Client rpcService);
  zap::HttpService::Client kjToZap(kj::Own<kj::HttpService> service);

  kj::HttpHeaders zapToKj(zap::List<zap::HttpHeader>::Reader zapHeaders) const;
  // Returned headers may alias into `zapHeaders`.

private:
  ByteStreamFactory& streamFactory;
  const kj::HttpHeaderTable& headerTable;
  OptimizationLevel peerOptimizationLevel;
  kj::Array<zap::CommonHeaderName> nameKjToZap;
  kj::Array<kj::HttpHeaderId> nameZapToKj;
  kj::Array<kj::StringPtr> valueZapToKj;
  kj::HashMap<kj::StringPtr, zap::CommonHeaderValue> valueKjToZap;

  class ZapToKjWebSocketAdapter;
  class KjToZapWebSocketAdapter;

  class ClientRequestContextImpl;
  class ConnectClientRequestContextImpl;
  class KjToZapHttpServiceAdapter;

  class HttpServiceResponseImpl;
  class HttpOverZapConnectResponseImpl;
  class ServerRequestContextImpl;
  class ZapToKjHttpServiceAdapter;

  zap::Orphan<zap::List<zap::HttpHeader>> headersToZap(
      const kj::HttpHeaders& headers, zap::Orphanage orphanage);
};

}  // namespace zap

ZAP_END_HEADER
