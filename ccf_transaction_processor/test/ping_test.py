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
import time

def run(args):

    num_pings = args.num_pings
    host = args.host
    port = 6006
    cert = "./user1_cert.pem"
    key = "./user1_privk.pem"
    cafile="./networkcert.pem"
    format = "json"

    client = CCFClient(host, port, cert=cert, key=key, ca = cafile, format=format, prefix="users", description="none", \
        version="2.0",connection_timeout=3, request_timeout=3)

    start_time = time.time()

    for _ in range(num_pings):
        client.rpc("ping", dict())

    end_time = time.time()

    total_time = end_time - start_time
    txn_throuput = num_pings/total_time

    print("Average txn_throuput is {} pings per second".format(txn_throuput))


if __name__ == "__main__":

    parser = argparse.ArgumentParser()

    parser.add_argument(
        "--num-pings",
        help="Number of ping operations to do",
        default = 100,
        type=int)

    parser.add_argument(
            "--host",
            help="IP address of the CCF service",
            type=str)

    args = parser.parse_args()
    run(args)
