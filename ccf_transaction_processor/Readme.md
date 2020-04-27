<!---
Licensed under Creative Commons Attribution 4.0 International License
https://creativecommons.org/licenses/by/4.0/
--->

# Microsoft CCF based PDO Transaction Processor

This folder contains software for PDO transaction processor (TP) based on Microsoft's CCF blockchain.
The software is located under $PDO_SOURCE_ROOT/transaction_processor/. The TP software is written and tested for CCF tag 0.9.2. Compatibility with other CCF versions is not guaranteed.

The instructions below may be followed to build and deploy CCF based PDO TP. Make sure the env variable PDO_SOURCE_ROOT
points to PDO local git repo directory.

## Get CCF Source Code

CCF tag 0.9.2 is included as a submodule within PDO. Download the submodule using the following commands:

```bash
cd $PDO_SOURCE_ROOT
git submodule init
git submodule update
```

## Install CCF Dependencies

CCF/PDO combo has been tested under a scenario where CCF is deployed in a standalone VM. Further, the CCF/PDO combo
has been tested only for the CCF virtual enclave mode. The dependencies needed to deploy CCF in an Ubuntu 18.04  VM
with  virtual enclave mode can be installed by running the following command:

```bash
cd $PDO_SOURCE_ROOT/ccf_transaction_processor/CCF/getting_started/setup_vm/
./setup_nodriver.sh
```

The above script works only when the VM is not behind any proxies. If there are proxies, make the following changes
before executing the above command:

A. In the setup_nosgx.sh file, modify the command `sudo add-apt-repository ppa:ansible/ansible -y` by adding the `-E` option. The modified command looks like `sudo -E add-apt-repository ppa:ansible/ansible -y`. Add a `sudo` prefix to the `ansible-playbook ccf-dependencies-no-driver.yml` command appearing in the same file.

B. In ccf-dependencies-no-driver.yml (present under the same folder), add the environment option for proxies
(see https://docs.ansible.com/ansible/latest/user_guide/playbooks_environment.html for reference)

```bash
environment:
    http_proxy: <specify-http-proxy-here>
    https_proxy: <specify-https-proxy-here>
```

C. Add the repo used by ansible scripts for installing python 3.7

```bash
sudo add-apt-repository ppa:deadsnakes/ppa
```

## Build CCF & PDO-TP

CCF uses a combination of cmake& ninja to build the application. Execute the following steps to complete the build process:

A. Add the following lines to the CMakeLists.txt found at $PDO_SOURCE_ROOT/ccf_transaction_processor/CCF.
    (look for other add_ccf_app in this file, add just above or below this)

```bash
add_ccf_app(
pdoenc SRCS ../transaction_processor/pdo_tp.cpp
../transaction_processor/verify_signatures.cpp
)
```

B. Create the build folder

```bash
mkdir $PDO_SOURCE_ROOT/ccf_transaction_processor/CCF/build
```

C. Set the build flags

```bash
cd $PDO_SOURCE_ROOT/ccf_transaction_processor/CCF/build
cmake -GNinja -DCOMPILE_TARGETS=virtual -DBUILD_END_TO_END_TESTS=OFF -DBUILD_SMALLBANK=OFF \
                -DBUILD_TESTS=OFF-DBUILD_UNIT_TESTS=OFF ..
```

D. Build using ninja

```bash
ninja
```

If the VM has a small memory (< 4GB), it might be required to reduce the parallelism in the build process using the `-j` flag for the `ninja` command, say ninja -j2 or even ninja -j1 (the default seems to be ninja -j4).

If build is successful, the libpdoenc.virtual.so library is created inside the build folder (ignore any other targets that get created)

## Deploy CCF with PDO-TP as the application

The following commands start a single node CCF under the virtual enclave mode.  PDO-TP is the hosted application.

```bash
cd $PDO_SOURCE_ROOT/ccf_transaction_processor/CCF/build
python3.7 -m venv env
source env/bin/activate
pip install -q -U -r ../tests/requirements.txt
python ../tests/start_network.py --gov-script ../src/runtime_config/gov.lua  --label pdo_tp -e virtual --package libpdoenc.virtual.so --node <ip-address:6006>
```

A single node CCF instance hosting the PDO TP will be available for business use @ ip-address:6006. Here `ip-address` is the address of this VM that can be used to ping this machine from other VMs (this is our use case). Set ip-address to 127.0.0.1 for local testing. If you are behind a proxy, add ip-address to the list of no_proxy bash env variable.

## Share CCF (TLS) Authentication Keys

CCF uses mutually authenticated TLS channels for transactions. Keys are located at
$PDO_SOURCE_ROOT/ccf_transaction_processor/CCF/build/workspace/pdo_tp_common. The network certificate is `networkcert.pem`. User public certificate is `user0_cert.pem` and private key is `user0_privk.pem`.
Note that the keys are created as part of CCF deployment and are unique to the specific instance of CCF.

In our usage, CCF users are PDO clients, and PDO client authentication is implemented within the transaction processor itself. Thus, we do not utilize the client authentication feature provided by CCF. However to satisfy the CCF's requirement that only authorized CCF users can submit transactions to CCF, share `user0_cert.pem` and `user0_privk.pem` with all the PDO clients. These two keys and the network certificate `networkcert.pem` must be stored under $PDO_LEDGER_KEY_ROOT (as part of PDO deployment). The user keys must be renamed as `userccf_cert.pem` and `userccf_privk.pem` respectively.

(The deployment script `start_network.py` generates to two additional pairs of user keys. We simply ignore these in our usage.)

## Test the Deployment with Ping Test

PDO TP contains a simple `ping` rpc that returns success every time it is queried. Test the PDO-TP deployment using this rpc. Invoke the following commands to issue 100 ping rpcs. The net througput is reported at the end of the test.

```bash
cd $PDO_SOURCE_ROOT/ccf_transaction_processor/test
./test.sh <ip-address>
```

The CCF port is assumed to be 6006.

## Generate Ledger Signing Keys for Signing Payloads of Read Transactions

Responses to read-transactions include a payload signature, where the signature is generated within PDO-TP.
The required signing keys must be generated before PDO-TP can be opened up for business from PDO clients.
Invoke the following commands to generate the signing keys.

```bash
cd $PDO_SOURCE_ROOT/ccf_transaction_processor/generate_signing_keys
./gen_keys.sh <ip-address>
```

If successful, the rpc returns after 1) creating one set of signing keys locally within the CCF enclave, and 2) scheduling them for global commit. The corresponding verifying key can be obtained using the `get_ledger_verifying_key`
rpc. Verifying key is returned only after global commit is successful. Invoke the following commands to
check that signing keys have been globally committed before opening up CCF for business from PDO clients.

```bash
./get_verifying_keys.sh <ip-address>
```

The read-payload-signature feature may be used by PDO clients to establish offline verifiable proof of transaction commits as desired by the PDO smart contract application. Note that for trust purposes, it is recommended that any entity that uses the verifying_key gets it directly from the CCF service using the `get_ledger_verifying_key` rpc.

## Using CCF ledger using PDO

We highlight some quick details about how PDO clients can use a CCF based PDO-TP deployment. The information below can be found at [PDO docs](../docs) as well.

1. Set the following environment variables:

```bash
export PDO_LEDGER_TYPE=ccf
export PDO_LEDGER_URL=http://ip-address:6006
```

Here, ip-address is the address of the node which hosts the CCF instance.

2. Set env PDO_LEDGER_KEY_ROOT and save CCF's network certificate `networkcert.pem` and user keys @ PDO_LEDGER_KEY_ROOT. Note that the user cert and private keys must be named as `userccf_cert.pem` and `userccf_privk.pem` respectively.

3. Do a clean build of PDO

```bash
cd $PDO_SOURCE_ROOT/build/
make clean
make
```
A clean build is an easy way to ensure updated creation of config files and PDO keys that are compatible with CCF.

4. Run unit tests
```bash
cd $PDO_SOURCE_ROOT/build/__tools__
./run-tests.sh
```
