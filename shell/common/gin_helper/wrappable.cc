// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#include "shell/common/gin_helper/wrappable.h"

#include "base/logging.h"
#include "base/threading/thread_task_runner_handle.h"
#include "shell/common/gin_helper/dictionary.h"

namespace gin_helper {

namespace {
static uint16_t kElectronEmbedderId = 0xE1ec;
}

WrappableBase::WrappableBase() = default;

WrappableBase::~WrappableBase() {
  if (wrapper_.IsEmpty())
    return;

  v8::HandleScope scope(isolate());
  GetWrapper()->SetAlignedPointerInInternalField(kSlot, nullptr);
  wrapper_.ClearWeak();
  wrapper_.Reset();
}

v8::Local<v8::Object> WrappableBase::GetWrapper() const {
  if (!wrapper_.IsEmpty())
    return v8::Local<v8::Object>::New(isolate_, wrapper_);
  else
    return v8::Local<v8::Object>();
}

v8::MaybeLocal<v8::Object> WrappableBase::GetWrapper(
    v8::Isolate* isolate) const {
  if (!wrapper_.IsEmpty())
    return v8::MaybeLocal<v8::Object>(
        v8::Local<v8::Object>::New(isolate, wrapper_));
  else
    return v8::MaybeLocal<v8::Object>();
}

void WrappableBase::InitWithArgs(gin::Arguments* args) {
  v8::Local<v8::Object> holder;
  args->GetHolder(&holder);
  InitWith(args->isolate(), holder);
}

void WrappableBase::InitWith(v8::Isolate* isolate,
                             v8::Local<v8::Object> wrapper) {
  CHECK(wrapper_.IsEmpty());
  isolate_ = isolate;
  wrapper->SetAlignedPointerInInternalField(kWrapperType, &kElectronEmbedderId);
  wrapper->SetAlignedPointerInInternalField(kSlot, this);
  wrapper_.Reset(isolate, wrapper);
  wrapper_.SetWeak(this, FirstWeakCallback,
                   v8::WeakCallbackType::kInternalFields);

  // Call object._init if we have one.
  v8::Local<v8::Function> init;
  if (Dictionary(isolate, wrapper).Get("_init", &init))
    init->Call(isolate->GetCurrentContext(), wrapper, 0, nullptr).IsEmpty();

  AfterInit(isolate);
}

// static
void WrappableBase::FirstWeakCallback(
    const v8::WeakCallbackInfo<WrappableBase>& data) {
  auto* wrappable = static_cast<WrappableBase*>(data.GetInternalField(kSlot));
  if (wrappable) {
    wrappable->wrapper_.Reset();
    data.SetSecondPassCallback(SecondWeakCallback);
  }
}

// static
void WrappableBase::SecondWeakCallback(
    const v8::WeakCallbackInfo<WrappableBase>& data) {
  delete static_cast<WrappableBase*>(data.GetInternalField(kSlot));
}

namespace internal {

void* FromV8Impl(v8::Isolate* isolate, v8::Local<v8::Value> val) {
  if (!val->IsObject())
    return nullptr;
  v8::Local<v8::Object> obj = val.As<v8::Object>();
  if (obj->InternalFieldCount() != WrappableBase::kInternalFieldCount ||
        &kElectronEmbedderId != obj->GetAlignedPointerFromInternalField(WrappableBase::kWrapperType))
    return nullptr;
  return obj->GetAlignedPointerFromInternalField(WrappableBase::kSlot);
}

}  // namespace internal

}  // namespace gin_helper
