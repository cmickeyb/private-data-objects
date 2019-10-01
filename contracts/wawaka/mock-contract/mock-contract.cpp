#include <malloc.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "Dispatch.h"

#include "KeyValue.h"
#include "Environment.h"
#include "Message.h"
#include "Response.h"
#include "StringArray.h"
#include "Value.h"
#include "WasmExtensions.h"

static KeyValueStore meta_store("meta");
static KeyValueStore value_store("values");
static KeyValueStore owner_store("owners");

const StringArray owner_key("owner");
const StringArray public_key_key("public-key");
const StringArray private_key_key("private-key");

bool initialize(const Message& msg, const Environment& env, Response& rsp)
{
    const StringArray owner_val(env.creator_id_);

    if (! meta_store.set(owner_key, owner_val))
    {
        rsp.set_error_result("failed to save creator metadata");
        return false;
    }

    StringArray public_key;
    StringArray private_key;

    if (! ecdsa_create_signing_keys((char**)&private_key.value_, &private_key.size_,
                                    (char**)&public_key.value_, &public_key.size_))
    {
        rsp.set_error_result("failed to create contract ecdsa keys");
        return false;
    }

    if (! meta_store.set(public_key_key, public_key))
    {
        rsp.set_error_result("failed to save public key");
        return false;
    }

    if (! meta_store.set(private_key_key, private_key))
    {
        rsp.set_error_result("failed to save private key");
        return false;
    }

    Value v(true);

    rsp.mark_state_modified();
    rsp.set_result(v.serialize());

    return true;
}

bool create(const Message& msg, const Environment& env, Response& rsp)
{
    const StringArray owner(env.message_id_);
    const StringArray key(msg.get_string("key"));
    const uint32_t value = (uint32_t)msg.get_number("value");

    uint32_t tmp;
    if (value_store.get(key, tmp))
    {
        rsp.set_error_result("duplicate key");
        return false;
    }

    if (! value_store.set(key, value))
    {
        rsp.set_error_result("failed to create the key");
        return false;
    }

    if (! owner_store.set(key, owner))
    {
        rsp.set_error_result("failed to save the owner");
        return false;
    }

    Value v((double)value);

    rsp.mark_state_modified();
    rsp.set_result(v.serialize());

    return true;
}

bool inc_value(const Message& msg, const Environment& env, Response& rsp)
{
    const StringArray key(msg.get_string("key"));

    uint32_t value;
    if (! value_store.get(key, value))
    {
        rsp.set_error_result("no such key");
        return false;
    }

    value += 1;
    if (! value_store.set(key, value))
    {
        rsp.set_error_result("failed to save the new value");
        return false;
    }

    Value v((double)value);

    rsp.mark_state_modified();
    rsp.set_result(v.serialize());

    return true;
}

bool get_value(const Message& msg, const Environment& env, Response& rsp)
{
    const StringArray key(msg.get_string("key"));

    uint32_t value;
    if (! value_store.get(key, value))
    {
        rsp.set_error_result("no such key");
        return false;
    }

    Value v((double)value);

    rsp.mark_state_unmodified();
    rsp.set_result(v.serialize());

    return true;
}

bool sign_message(const Message& msg, const Environment& env, Response& rsp)
{
    const StringArray message(msg.get_string("message"));

    StringArray private_key;
    if (! meta_store.get(private_key_key, private_key))
    {
        rsp.set_error_result("failed to find private key");
        return false;
    }

    StringArray signature;
    if (! ecdsa_sign_message(message.value_, message.size_,
                             (const char*)private_key.value_, private_key.size_,
                             &signature.value_, &signature.size_))
    {
        rsp.set_error_result("failed to sign message");
        return false;
    }

    StringArray encoded;
    if (! b64_encode(signature.value_, signature.size_, (char**)&encoded.value_, &encoded.size_))
    {
        rsp.set_error_result("failed to encode signature");
        return false;
    }

    Value v(encoded.value_);

    rsp.mark_state_unmodified();
    rsp.set_result(v.serialize());

    return true;
}

contract_method_reference_t contract_method_dispatch_table[] = {
    CONTRACT_METHOD(initialize),
    CONTRACT_METHOD(create),
    CONTRACT_METHOD(inc_value),
    CONTRACT_METHOD(get_value),
    CONTRACT_METHOD(sign_message),
    { NULL, NULL }
};
