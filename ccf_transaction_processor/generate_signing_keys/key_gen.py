# Copyright 2020 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from infra.clients import CCFClient
import argparse

def run(args):
    
    host = args.host
    port = 6006
    cert = "./user1_cert.pem"
    key = "./user1_privk.pem"
    cafile="./networkcert.pem"
    format = "json"

    client = CCFClient(host, port, cert=cert, key=key, ca = cafile, format=format, prefix="users", description="none", \
        version="2.0",connection_timeout=3, request_timeout=3)

    r=client.rpc("generate_signing_key_for_read_payloads", dict())
    if r.result:
       print(r.result)
    else:
       print(r.error['message'])

if __name__ == "__main__":

    parser = argparse.ArgumentParser()

    parser.add_argument(
            "--host",
            help="IP address of the CCF service",
            type=str)

    args = parser.parse_args()
    run(args)
