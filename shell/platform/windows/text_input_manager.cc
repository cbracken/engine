// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/windows/text_input_manager.h"

#include <imm.h>

namespace flutter {

// RAII wrapper for IMM32 IMM contexts.
//
// Gets the current IMM context on construction and releases it on destruction.
class ImmContext {
 public:
  explicit ImmContext(HWND window_handle) noexcept;
  ~ImmContext();

  ImmContext(const ImmContext&) = delete;
  ImmContext& operator=(const ImmContext&) = delete;

  // Returns the IMM context handle.
  HIMC get() const;

 private:
  HWND window_handle_ = nullptr;
  HIMC imm_context_ = nullptr;
};

ImmContext::ImmContext(HWND window_handle) noexcept : window_handle_(window_handle) {
  if (window_handle_ != nullptr) {
    imm_context_ = ::ImmGetContext(window_handle_);
  }
}

ImmContext::~ImmContext() {
  if (window_handle_ != nullptr && imm_context_ != nullptr) {
    ::ImmReleaseContext(window_handle_, imm_context_);
  }
}

HIMC ImmContext::get() const {
  return imm_context_;
}


void TextInputManager::SetWindowHandle(HWND window_handle) {
  window_handle_ = window_handle;
}

void TextInputManager::CreateImeWindow() {
  if (window_handle_ == nullptr) {
    return;
  }

  // Some IMEs ignore calls to ::ImmSetCandidateWindow() and use the position of
  // the current system caret instead via ::GetCaretPos(). In order to behave
  // as expected with these IMEs, we create a temporary system caret.
  if (!ime_active_) {
    ::CreateCaret(window_handle_, nullptr, 1, 1);
  }
  ime_active_ = true;

  // Set the position of the IME windows.
  UpdateImeWindow();
}

void TextInputManager::DestroyImeWindow() {
  if (window_handle_ == nullptr) {
    return;
  }

  // Destroy the system caret created in CreateImeWindow().
  if (ime_active_) {
    ::DestroyCaret();
  }
  ime_active_ = false;
}

void TextInputManager::UpdateImeWindow() {
  if (window_handle_ == nullptr) {
    return;
  }
  MoveImeWindow();
}

void TextInputManager::UpdateCaretRect(const Rect& rect) {
  caret_rect_ = rect;

  if (window_handle_ == nullptr) {
    return;
  }
  MoveImeWindow();
}

long TextInputManager::GetComposingCursorPosition() const {
  if (window_handle_ == nullptr) {
    return false;
  }

  ImmContext imm_context(window_handle_);
  if (imm_context.get()) {
    // Read the cursor position within the composing string.
    const int pos =
        ImmGetCompositionStringW(imm_context.get(), GCS_CURSORPOS, nullptr, 0);
    return pos;
  }
  return -1;
}

std::optional<std::u16string> TextInputManager::GetComposingString() const {
  return GetString(GCS_COMPSTR);
}

std::optional<std::u16string> TextInputManager::GetResultString() const {
  return GetString(GCS_RESULTSTR);
}

std::optional<std::u16string> TextInputManager::GetString(int type) const {
  if (window_handle_ == nullptr || !ime_active_) {
    return std::nullopt;
  }
  ImmContext imm_context(window_handle_);
  if (imm_context.get()) {
    // Read the composing string length.
    const long compose_bytes =
        ::ImmGetCompositionString(imm_context.get(), type, nullptr, 0);
    const long compose_length = compose_bytes / sizeof(wchar_t);
    if (compose_length <= 0) {
      return std::nullopt;
    }

    std::u16string text(compose_length, '\0');
    ::ImmGetCompositionString(imm_context.get(), type, &text[0], compose_bytes);
    return text;
  }
  return std::nullopt;
}

void TextInputManager::MoveImeWindow() {
  if (GetFocus() != window_handle_ || !ime_active_) {
    return;
  }
  LONG x = caret_rect_.left();
  LONG y = caret_rect_.top();
  ::SetCaretPos(x, y);

  ImmContext imm_context(window_handle_);
  if (imm_context.get()) {
    COMPOSITIONFORM cf = {CFS_POINT, {x, y}};
    ::ImmSetCompositionWindow(imm_context.get(), &cf);

    CANDIDATEFORM candidate_form = {0, CFS_CANDIDATEPOS, {x, y}, {0, 0, 0, 0}};
    ::ImmSetCandidateWindow(imm_context.get(), &candidate_form);
  }
}

}  // namespace flutter
