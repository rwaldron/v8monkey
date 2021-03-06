#include "v8-internal.h"

namespace v8 {
using namespace internal;

namespace internal {
  template <typename SlotOps>
  class ReferenceContainer {
    static const size_t kBlockSize = 16;
    struct SlotBlock {
      typename SlotOps::Slot elements[kBlockSize];
      SlotBlock *next;
    };
    SlotBlock *mBlock;
    size_t mUsedThisBlock;
    void deallocateBlock() {
      if (!mBlock)
        return;

      SlotBlock *next = mBlock->next;

      while (mUsedThisBlock--)
        SlotOps::onRemoveSlot(&mBlock->elements[mUsedThisBlock]);

      delete mBlock;
      mUsedThisBlock = kBlockSize;
      mBlock = next;
    }
  public:
    ReferenceContainer() :
      mBlock(NULL),
      mUsedThisBlock(0)
    {}
    typename SlotOps::Slot *allocate() {
      if (mUsedThisBlock == kBlockSize || !mBlock) {
        SlotBlock *block = new SlotBlock;
        block->next = mBlock;
        mBlock = block;
        mUsedThisBlock = 0;
      }
      typename SlotOps::Slot *slot = &mBlock->elements[mUsedThisBlock];
      mUsedThisBlock++;
      SlotOps::onNewSlot(slot);
      return slot;
    }
    bool containsSlot(typename SlotOps::Slot *slot) {
      SlotBlock *current = mBlock;
      while (current) {
        // XXX: this may rely on undefined behavior in C's memory model
        if (slot >= current->elements || slot <= &current->elements[kBlockSize-1]) {
          return true;
        }
        current = current->next;
      }
      return false;
    }
    ~ReferenceContainer() {
      while (mBlock) {
        deallocateBlock();
      }
    }
  };

  struct GCOps {
    typedef internal::GCReference Slot;
    static void onNewSlot(Slot *s) {
      s->root(cx());
    }
    static void onRemoveSlot(Slot *s) {
      s->unroot(cx());
    }
  };

  class GCReferenceContainer : public ReferenceContainer<GCOps> {};

  PersistentGCReference::PersistentGCReference(GCReference *ref) :
    GCReference(*ref),
    callback(NULL), context(NULL),
    prev(NULL), next(NULL)
  {}

  void PersistentGCReference::MakeWeak(WeakReferenceCallback callback, void *context) {
    this->callback = callback;
    this->context = context;
    next = weakPtrs;
    prev = NULL;
    weakPtrs = this;
    unroot(cx());
  }

  void PersistentGCReference::ClearWeak() {
    if (next)
      next->prev = prev;
    if (prev)
      prev->next = next;
    if (weakPtrs == this)
      weakPtrs = next;
    prev = next = NULL;
    root(cx());
  }

  PersistentGCReference *PersistentGCReference::weakPtrs = NULL;

  void PersistentGCReference::CheckForWeakHandles() {
    PersistentGCReference *ref = weakPtrs;
    while (ref != NULL) {
      jsval v = ref->native();
      ref->isNearDeath = JSVAL_IS_GCTHING(v) == JS_TRUE &&
          JS_IsAboutToBeFinalized(cx(), JSVAL_TO_GCTHING(v)) == JS_TRUE;
      if (ref->isNearDeath && ref->callback) {
        Persistent<Value> h(reinterpret_cast<Value*>(ref));
        ref->callback(h, ref->context);
      }
      ref = ref->next;
    }
  }

  GCReference* GCReference::Globalize() {
    GCReference *r = new PersistentGCReference(this);
    r->root(cx());
    return r;
  }

  void GCReference::Dispose() {
      unroot(cx());
      // Yay no RTTI
      if (HandleScope::IsLocalReference(this)) {
        delete this;
      } else {
        PersistentGCReference *ref =
          reinterpret_cast<PersistentGCReference*>(this);
        delete ref;
      }
  }

  GCReference *GCReference::Localize() {
    return HandleScope::CreateHandle(*this);
  }
}

HandleScope *HandleScope::sCurrent = 0;

HandleScope::HandleScope() :
  mGCReferences(new internal::GCReferenceContainer),
  mPrevious(sCurrent)
{
  sCurrent = this;
}

HandleScope::~HandleScope() {
  Destroy();
}

internal::GCReference* HandleScope::InternalClose(internal::GCReference* ref) {
  JS_ASSERT(mPrevious);
  JS_ASSERT(mGCReferences);
  JS_ASSERT(sCurrent == this);
  jsval v = ref->native();
  Destroy();
  return HandleScope::CreateHandle(v);
}

void HandleScope::Destroy() {
  if (sCurrent == this) {
    sCurrent = mPrevious;
    delete mGCReferences;
    mGCReferences = NULL;
  }
}

internal::GCReference *HandleScope::CreateHandle(internal::GCReference r) {
  internal::GCReference *ref = sCurrent->mGCReferences->allocate();
  *ref = r;
  return ref;
}

bool HandleScope::IsLocalReference(GCReference *ref) {
  HandleScope *current = sCurrent;
  while (current != NULL) {
    GCReferenceContainer *container = current->mGCReferences;
    if (container->containsSlot(ref)) {
      return true;
    }
    current = current->mPrevious;
  }
  return false;
}

}
