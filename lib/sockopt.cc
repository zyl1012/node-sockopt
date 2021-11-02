#define NAPI_EXPERIMENTAL
#include <napi.h>
#include <errno.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <node.h>

#include "uv.h"

uv_handle_t* getTcpHandle(void *handleWrap) {
    volatile char *memory = (volatile char *) handleWrap;
    for (volatile uv_handle_t *tcpHandle = (volatile uv_handle_t *) memory; tcpHandle->type != UV_TCP
         || tcpHandle->data != handleWrap || tcpHandle->loop != uv_default_loop(); tcpHandle = (volatile uv_handle_t *) memory) {
        memory++;
    }
    return (uv_handle_t *) memory;
}
static v8::Local<v8::Value> V8LocalValueFromJsValue(napi_value v)
{
  v8::Local<v8::Value> local;
  memcpy(static_cast<void*>(&local), static_cast<void*>(&v), sizeof(v));
  return local;
}

SOCKET getAddress(const Napi::CallbackInfo& args) {
	Napi::Object object = args[0].As<Napi::Object>();
	napi_value _handle = args[0];
	v8::Local<v8::Value> value = V8LocalValueFromJsValue(_handle);
	v8::Local<v8::Object> v8object = value.As<v8::Object>();
	void *handleWrap = v8object->GetAlignedPointerFromInternalField(0);
	uv_handle_t* handle = getTcpHandle(handleWrap);  
	uv_tcp_t* tcpHandle = (uv_tcp_t*)handle;	
	return tcpHandle->socket;
}

#else
#include <sys/types.h>
#include <sys/socket.h>
#endif


Napi::Value Getsockopt(const Napi::CallbackInfo& args) {
	Napi::Env env = args.Env();

	if (!args[1].IsNumber() || !args[2].IsNumber()) {
		Napi::TypeError::New(env, "Wrong arguments").ThrowAsJavaScriptException();
		return env.Null();
	}

	int level = args[1].As<Napi::Number>().Int32Value();
	int name = args[2].As<Napi::Number>().Int32Value();


	int * val = (int *) malloc(sizeof(int));
	socklen_t len = sizeof(int);
#ifdef _WIN32
	SOCKET socket = getAddress(args);
	if (0 != getsockopt(socket, level, name, (char *) val, &len)) {
		char * msg = (char *) malloc(sizeof(char) * 100);
		sprintf(msg, "getsockopt failed: socket:%llu %s (%d)",  socket, strerror(errno), errno);
		// todo: expose `errno`
		Napi::TypeError::New(env, msg).ThrowAsJavaScriptException();
		return env.Null();
	}
#else
	int socket = args[0].As<Napi::Number>().Int32Value();
	if (0 != getsockopt(socket, level, name, (void *) val, &len)) {
		char * msg = (char *) malloc(sizeof(char) * 100);
		sprintf(msg, "getsockopt failed: %s (%d)", strerror(errno), errno);
		// todo: expose `errno`
		Napi::TypeError::New(env, msg).ThrowAsJavaScriptException();
		return env.Null();
	}
#endif
	return Napi::Number::New(env, * val);
}

Napi::Value Setsockopt(const Napi::CallbackInfo& args) {
	Napi::Env env = args.Env();

	if (args.Length() < 2) {
		Napi::TypeError::New(env, "Wrong number of arguments")
				.ThrowAsJavaScriptException();
		return env.Null();
	}

	if (!args[1].IsNumber() || !args[2].IsNumber() || !args[3].IsNumber()) {
		Napi::TypeError::New(env, "Wrong arguments").ThrowAsJavaScriptException();
		return env.Null();
	}

	
	int level = args[1].As<Napi::Number>().Int32Value();
	int name = args[2].As<Napi::Number>().Int32Value();
	int new_val = args[3].As<Napi::Number>().Int32Value();

#ifdef _WIN32
	SOCKET socket = getAddress(args);
	if (0 != setsockopt(socket, level, name, (const char *)&new_val, (socklen_t) sizeof(new_val))) {
		char * msg = (char *) malloc(sizeof(char) * 100);
		sprintf(msg, "getsockopt failed: socket:%llu %s (%d)",  socket, strerror(errno), errno);
		// todo: expose `errno`
		Napi::TypeError::New(env, msg).ThrowAsJavaScriptException();
	}
#else
	int socket = args[0].As<Napi::Number>().Int32Value();
	if (0 != setsockopt(socket, level, name, (void *)&new_val, (socklen_t) sizeof(new_val))) {
		char * msg = (char *) malloc(sizeof(char) * 100);
		sprintf(msg, "setsockopt failed: %s (%d)", strerror(errno), errno);
		// todo: expose `errno`
		Napi::TypeError::New(env, msg).ThrowAsJavaScriptException();
	}
#endif
	return env.Null();
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
	exports.Set(Napi::String::New(env, "getsockopt"), Napi::Function::New(env, Getsockopt));
	exports.Set(Napi::String::New(env, "setsockopt"), Napi::Function::New(env, Setsockopt));
	return exports;
}

NODE_API_MODULE(addon, Init)
