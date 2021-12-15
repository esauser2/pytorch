#include <c10/core/DispatchKeySet.h>

namespace c10 {

// autograd_dispatch_keyset should include all runtime autograd keys.
// Alias key DispatchKey::Autograd maps to autograd_dispatch_keyset.
// NB: keys in this set also get associated with CompositeImplicitAutograd
DispatchKeySet _autograd_dispatch_keyset() {
    static DispatchKeySet autograd_dispatch_keyset = DispatchKeySet({
        DispatchKey::AutogradFunctionality,
        DispatchKey::AutogradOther,
    }) | DispatchKeySet(DispatchKeySet::RAW, full_backend_mask);
    return autograd_dispatch_keyset;
}

DispatchKeySet _autocast_dispatch_keyset() {
    static DispatchKeySet autocast_dispatch_keyset = DispatchKeySet({
        DispatchKey::AutocastCPU,
        DispatchKey::AutocastCUDA,
    });
    return autocast_dispatch_keyset;
}

// See Note [TLS Initialization]
DispatchKeySet _default_included_set() {
    static DispatchKeySet default_included_set = DispatchKeySet({
        DispatchKey::BackendSelect,
        DispatchKey::ADInplaceOrView,
    });
    return default_included_set;
}

DispatchKeySet _default_excluded_set() {
    static DispatchKeySet default_excluded_set = DispatchKeySet({
        DispatchKey::AutocastCPU,
        DispatchKey::AutocastCUDA,
    });
    return default_excluded_set;
}

DispatchKeySet _autograd_dispatch_keyset_with_ADInplaceOrView() {
    static DispatchKeySet autograd_dispatch_keyset_with_ADInplaceOrView =
        autograd_dispatch_keyset | DispatchKeySet(DispatchKey::ADInplaceOrView);
    return autograd_dispatch_keyset_with_ADInplaceOrView;
}

// backend dispatch keys that map to DispatchKey::AutogradOther
// NB: keys in this set also get associated with CompositeImplicitAutograd
DispatchKeySet _autogradother_backends() {
    static DispatchKeySet autogradother_backends = DispatchKeySet(
        // TODO: delete commented code before landing.
        // HIP and VE now have their own backend bits, which means that
        // they can now have their own Autograd keys.
        // Technically, HIP will now redispatch to its own custom AutogradHIP slot
        // in the runtime table.
        //{DispatchKey::HIP,
        //DispatchKey::VE,
        {DispatchKey::FPGA,
        DispatchKey::ORT,
        DispatchKey::Vulkan,
        DispatchKey::Metal,
        DispatchKey::SparseCsrCPU,
        DispatchKey::SparseCsrCUDA,
        DispatchKey::CustomRNGKeyId,
        DispatchKey::MkldnnCPU,
        DispatchKey::Meta});
    return autogradother_backends;
}

// The set of dispatch keys that come after autograd
// n.b. this relies on the fact that AutogradOther is currently the lowest
// Autograd key
DispatchKeySet _after_autograd_keyset() {
    static DispatchKeySet after_autograd_keyset =
        DispatchKeySet(DispatchKeySet::FULL_AFTER, c10::DispatchKey::AutogradOther);
    return after_autograd_keyset;
}

// The set of dispatch keys that come after ADInplaceOrView
DispatchKeySet _after_ADInplaceOrView_keyset() {
    static DispatchKeySet after_ADInplaceOrView_keyset = DispatchKeySet(
        DispatchKeySet::FULL_AFTER,
        c10::DispatchKey::ADInplaceOrView);
    return after_ADInplaceOrView_keyset;
}

// The set of dispatch keys that come after Functionalize
DispatchKeySet _after_func_keyset() {
    static DispatchKeySet after_func_keyset =
        DispatchKeySet(DispatchKeySet::FULL_AFTER, c10::DispatchKey::Functionalize)
            .removeFunctionalityKey(
                // NOTE: we also need to remove ADInplaceOrView from the keyset when
                // redispatching after the func kernels. This is because we're not
                // calling the same op; we originally called an inplace op, and now
                // we aren't. The original key calculation figured out which keys
                // were Fallthrough based on the inplace op. That means that it did
                // not include the ADInPlaceOrView kernel as a fallthrough key.
                // However, we WANT the ADInPlaceOrView kernel to be ignored now
                // that we're calling an out-of-place op. Re-invoking
                // Dispatcher::call would re-run the Fallthrough key calculation and
                // get us that, But at::redispatch is more performant. We can get
                // away with it by explicitly removing the key here.
                c10::DispatchKey::ADInplaceOrView);
    return after_func_keyset;
}

// backend_dispatch_keyset should include all runtime backend keys.
// Alias key DispatchKey::CompositeExplicitAutograd maps to
// backend_dispatch_keyset NestedTensor has been explicitly removed due to
// incompatibility with some kernels, such as structured kernels, that use the
// DefaultBackend key.
DispatchKeySet _backend_dispatch_keyset() {
    static DispatchKeySet backend_dispatch_keyset = autogradother_backends |
        DispatchKeySet(DispatchKeySet::RAW, full_backend_mask) |
        DispatchKeySet({
            DispatchKey::Dense,
            DispatchKey::Sparse,
            DispatchKey::Quantized,
        });
    return backend_dispatch_keyset;
}

// math_dispatch_keyset contains all keys in backend_dispatch_keyset and
// autograd_dispatch_keyset Alias key DispatchKey::CompositeImplicitAutograd
// maps to math_dispatch_keyset.
DispatchKeySet _math_dispatch_keyset() {
    static DispatchKeySet math_dispatch_keyset =
        backend_dispatch_keyset | autograd_dispatch_keyset;
    return math_dispatch_keyset;
}

DispatchKeySet autograd_dispatch_keyset = _autograd_dispatch_keyset();
DispatchKeySet autocast_dispatch_keyset = _autocast_dispatch_keyset();
DispatchKeySet default_included_set = _default_included_set();
DispatchKeySet default_excluded_set = _default_excluded_set();
DispatchKeySet autograd_dispatch_keyset_with_ADInplaceOrView = _autograd_dispatch_keyset_with_ADInplaceOrView();
DispatchKeySet autogradother_backends = _autogradother_backends();
DispatchKeySet after_autograd_keyset = _after_autograd_keyset();
DispatchKeySet after_ADInplaceOrView_keyset = _after_ADInplaceOrView_keyset();
DispatchKeySet after_func_keyset = _after_func_keyset();
DispatchKeySet backend_dispatch_keyset = _backend_dispatch_keyset();
DispatchKeySet math_dispatch_keyset = _math_dispatch_keyset();

bool isBackendDispatchKey(DispatchKey t) {
  return t != DispatchKey::Undefined
      // See Note [No Alias Keys in DispatchKeySet]
      && !isAliasDispatchKey(t) && backend_dispatch_keyset.has(t);
}

DispatchKeySet getRuntimeDispatchKeySet(DispatchKey t) {
  TORCH_INTERNAL_ASSERT(t != DispatchKey::Undefined);
  switch (t) {
    case DispatchKey::Autograd:
      return autograd_dispatch_keyset;
    case DispatchKey::CompositeImplicitAutograd:
      return math_dispatch_keyset;
    case DispatchKey::CompositeExplicitAutograd:
      return backend_dispatch_keyset;
    default:
      return DispatchKeySet(t);
  }
}

bool runtimeDispatchKeySetHas(DispatchKey t, DispatchKey k) {
  TORCH_INTERNAL_ASSERT(t != DispatchKey::Undefined);
  switch (t) {
    case DispatchKey::Autograd:
      return autograd_dispatch_keyset.has(k);
    case DispatchKey::CompositeImplicitAutograd:
      return math_dispatch_keyset.has(k);
    case DispatchKey::CompositeExplicitAutograd:
      return backend_dispatch_keyset.has(k);
    default:
      return t == k;
  }
}

// for a given autograd key, return the (guaranteed nonempty) set of associated
// backend keys. for a non-autograd key, return the empty keyset.
DispatchKeySet getBackendKeySetFromAutograd(DispatchKey t) {
  switch (t) {
    case DispatchKey::AutogradCPU:
      return DispatchKeySet(DispatchKey::CPU);
    case DispatchKey::AutogradCUDA:
      return DispatchKeySet(DispatchKey::CUDA);
    case DispatchKey::AutogradXLA:
      return DispatchKeySet(DispatchKey::XLA);
    case DispatchKey::AutogradLazy:
      return DispatchKeySet(DispatchKey::Lazy);
    case DispatchKey::AutogradMLC:
      return DispatchKeySet(DispatchKey::MLC);
    case DispatchKey::AutogradHPU:
      return DispatchKeySet(DispatchKey::HPU);
    case DispatchKey::AutogradNestedTensor:
      return DispatchKeySet(DispatchKey::NestedTensor);
    case DispatchKey::AutogradXPU:
      return DispatchKeySet(DispatchKey::XPU);
    case DispatchKey::AutogradPrivateUse1:
      return DispatchKeySet(DispatchKey::PrivateUse1);
    case DispatchKey::AutogradPrivateUse2:
      return DispatchKeySet(DispatchKey::PrivateUse2);
    case DispatchKey::AutogradPrivateUse3:
      return DispatchKeySet(DispatchKey::PrivateUse3);
    case DispatchKey::AutogradOther:
      return autogradother_backends;
    default:
      return DispatchKeySet();
  }
}

DispatchKeySet getAutocastRelatedKeySetFromBackend(DispatchKey t) {
  TORCH_INTERNAL_ASSERT(t <= DispatchKey::EndOfBackendKeys || t == DispatchKey::Undefined);
  switch (t) {
    case DispatchKey::CPUBit:
      return DispatchKeySet(DispatchKey::AutocastCPU);
    case DispatchKey::CUDABit:
    case DispatchKey::XLABit:
      return DispatchKeySet(DispatchKey::AutocastCUDA);
    default:
      return DispatchKeySet();
  }
}

DispatchKeySet getAutogradRelatedKeySetFromBackend(DispatchKey t) {
  return DispatchKeySet(
      {DispatchKey::ADInplaceOrView, getAutogradKeyFromBackend(t)});
}

bool isIncludedInAlias(DispatchKey k, DispatchKey alias) {
  return k != DispatchKey::Undefined && runtimeDispatchKeySetHas(alias, k);
}

std::string toString(DispatchKeySet ts) {
  std::stringstream ss;
  ss << ts;
  return ss.str();
}

std::ostream& operator<<(std::ostream& os, DispatchKeySet ts) {
  if (ts.empty()) {
    os << "DispatchKeySet()";
    return os;
  }
  os << "DispatchKeySet(";
  bool first = true;
  for (auto k : ts) {
    if (!first) {
      os << ", ";
    }
    os << k;
    first = false;
  }
  os << ")";
  return os;
}

DispatchKeySet::iterator& DispatchKeySet::iterator::operator++() {
      TORCH_INTERNAL_ASSERT(
          functionality_mask_ > static_cast<uint8_t>(DispatchKey::EndOfBackendKeys));
      TORCH_INTERNAL_ASSERT(
          functionality_mask_ <= static_cast<uint8_t>(DispatchKey::EndOfFunctionalityKeys));
      TORCH_INTERNAL_ASSERT(
          backend_mask_ <= static_cast<uint8_t>(DispatchKey::EndOfBackendKeys));

      // Create a masked version of the set representation to ignore previous
      // keys that we've iterated through.
      uint64_t masked_functionality_bits = llvm::maskTrailingZeros<uint64_t>(functionality_mask_) & *data_ptr_;
      uint64_t masked_backend_bits = llvm::maskTrailingZeros<uint64_t>(backend_mask_) & full_backend_mask & *data_ptr_;

      uint64_t first_functionality_idx = llvm::findFirstSet(masked_functionality_bits);
      uint64_t first_backend_idx = llvm::findFirstSet(masked_backend_bits);

      // If there are no keys, set to end iterator value
      if (first_functionality_idx == std::numeric_limits<uint64_t>::max() ||
          functionality_mask_ == static_cast<uint8_t>(DispatchKey::EndOfFunctionalityKeys)) {
        // Set up state to be the same as end()
        functionality_mask_ = static_cast<uint8_t>(DispatchKey::EndOfFunctionalityKeys);
        functionality_idx_ = static_cast<uint8_t>(DispatchKey::EndOfFunctionalityKeys);
        backend_mask_ = 0;
        backend_idx_ = static_cast<uint8_t>(DispatchKey::EndOfFunctionalityKeys);
        return *this;
      }

      // If the current functionality bit is a per-backend bit, we need special handling
      if (isPerBackendFunctionalityKey(static_cast<DispatchKey>(first_functionality_idx + 1))) {
        // case 1: if the current backend is undefined, then there is no valid backend instance
        // of this functionality key so we can skip it.
        if (first_backend_idx == std::numeric_limits<uint64_t>::max()) {
          // increment the functionality mask so we skip the current functionality bit on the next increment.
          functionality_mask_ = static_cast<uint8_t>(first_functionality_idx) + 1;
          ++(*this);
          return *this;
        }

        // Otherwise, at this point we know what the current backend and functionality bits are.
        // (The +1 for the functionality idx but not the backend idx is because of the Undefined key)
        functionality_idx_ = static_cast<uint8_t>(first_functionality_idx) + 1;
        backend_idx_ = static_cast<uint8_t>(first_backend_idx);

        // Next, we need to set up the masks for the next increment.
        uint64_t next_backend_bits = llvm::maskTrailingZeros<uint64_t>(first_backend_idx + 1) & full_backend_mask & *data_ptr_;
        uint64_t next_backend_idx = llvm::findFirstSet(next_backend_bits);
        if (next_backend_idx == std::numeric_limits<uint64_t>::max()) {
          // case 2: the current backend is valid, but there is not another backend in the keyset.
          // In this case, we need to bump the functionality mask and reset the backend mask for the next increment
          functionality_mask_ = static_cast<uint8_t>(first_functionality_idx) + 1;
          backend_mask_ = 0;
        } else {
          // case 3: we have another backend to iterate over. We want to iterate over the same functionality bit
          // next time, but a different backend bit.
          backend_mask_ = static_cast<uint8_t>(first_backend_idx) + 1;
        }
      } else {
          // Functionality bits that aren't per backend are simpler to handle. We can ignore the backend bits.
          TORCH_INTERNAL_ASSERT(backend_mask_ == 0);
          functionality_idx_ = static_cast<uint8_t>(first_functionality_idx) + 1;
          functionality_mask_ = static_cast<uint8_t>(first_functionality_idx) + 1;
      }
      return *this;
    }

std::array<FunctionalityOffsetAndMask, num_functionality_keys>
initializeFunctionalityOffsetsAndMasks() {
    std::array<FunctionalityOffsetAndMask, num_functionality_keys>
    offsets_and_masks;
    // manual set the first entry, which corresponds to Undefined.
    offsets_and_masks[0] = FunctionalityOffsetAndMask(0, 0);
    // loop through every functionality key (aside from Undefined).
    for (uint8_t k_idx = static_cast<uint8_t>(DispatchKey::EndOfBackendKeys) + 2;
            k_idx < static_cast<uint8_t>(DispatchKey::EndOfFunctionalityKeys); ++k_idx) {
        // functionality_idx should be Dense -> 1, ...
        auto functionality_idx = k_idx - static_cast<uint8_t>(DispatchKey::EndOfBackendKeys) - 1;
        auto prev_offset_and_mask = offsets_and_masks[functionality_idx - 1];
        auto k = static_cast<DispatchKey>(k_idx);

#if defined(C10_MOBILE_TRIM_DISPATCH_KEYS)
// [Note: Trimmed Mobile Dispatch Keys]
        uint16_t mask = 0;
        uint16_t offset = -1;
        switch (k) {
          case DispatchKey::Undefined:
            offset = 0;
          case DispatchKey::CPU:
            offset = 1;
          case DispatchKey::QuantizedCPU:
            offset = 2;
          case DispatchKey::SparseCPU:
            offset = 3;
          case DispatchKey::BackendSelect:
            offset = 4;
          case DispatchKey::ADInplaceOrView:
            offset = 5;
          case DispatchKey::AutogradOther:
            offset = 6;
          case DispatchKey::AutogradCPU:
            offset = 7;
          default:
            offset = -1;
        }
        offsets_and_masks[functionality_idx] = FunctionalityOffsetAndMask(offset, 0);
#else
        // If the previous functionality was not per-backend, then we can just increment the previous offset.
        // Otherwise, the next offset = previous_offset + num_backends.
        auto next_offset = prev_offset_and_mask.offset +
            (prev_offset_and_mask.mask == 0 ? 1 : (static_cast<uint8_t>(DispatchKey::EndOfBackendKeys) + 1));
        // the mask is used in the runtime index calculation to find the offset of the backend.
        // For non-per-backend functionalities, this offset should always be 0.
        // Otherwise, we need to get the index of the backend (which we can do using a backend mask).
        auto next_mask = isPerBackendFunctionalityKey(k) ? full_backend_mask : 0;
        offsets_and_masks[functionality_idx] = FunctionalityOffsetAndMask(next_offset, next_mask);
    }
    // Sanity check that the computed offset index of the last functionality key is correct.
    // This assumes that the highest priority functionality key is not per backend.
    TORCH_INTERNAL_ASSERT(offsets_and_masks[num_functionality_keys - 1].offset == (num_runtime_entries - 1),
        "num_runtime_entries: ", num_runtime_entries,
        "last_offset: ", offsets_and_masks[num_functionality_keys - 1].offset);
#endif
    return offsets_and_masks;
}

} // namespace c10
