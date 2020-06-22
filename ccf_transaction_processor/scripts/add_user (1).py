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

import argparse
import http
import os
import sys

sys.path.insert(1, '../CCF/tests/infra')
from clients import CCFClient

def run(args):

    host = args.host
    port = 6006
    cert = "./{}_cert.pem".format(args.member_name)
    key = "./{}_privk.pem".format(args.member_name)
    cafile="./networkcert.pem"
    format = "json"

    user_cert = "./{}_cert.pem".format(args.user_name)

    client = CCFClient(host, port, cert=cert, key=key, ca = cafile, format=format, prefix="members", \
        description="none", version="2.0",connection_timeout=3, request_timeout=3)

    with open(os.path.join("./", user_cert)) as usercertfile:
        user_cert_unicode = [ord(c) for c in usercertfile.read()]

    script = """
    tables, user_cert = ...
    return Calls:call("new_user", user_cert)
    """
    rpc_params = dict()
    rpc_params['parameter'] = user_cert_unicode
    rpc_params['script'] = {"text": script}
    rpc_params['ballot'] = {"text": "return true"}

    r = client.rpc("propose",rpc_params,signed=True)

    if r.status != http.HTTPStatus.OK.value:
        print("Error: Failed to add user: {}".format(r.status))

    print("Successfully created proposal to add user with proposal id {}".format(r.result["proposal_id"]))

if __name__ == "__main__":

    parser = argparse.ArgumentParser()

    parser.add_argument(
        "--member-name",
        help="Name of the member adding the user",
        default = "member0",
        type=str)

    parser.add_argument(
        "--user-name",
        help="Name of the user being added",
        default = "user0",
        type=str)

    parser.add_argument(
        "--host",
         help="IP address of the CCF service",
         type=str)

    args = parser.parse_args()
    run(args)
