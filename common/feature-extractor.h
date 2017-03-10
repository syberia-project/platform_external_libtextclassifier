/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Generic feature extractor for extracting features from objects. The feature
// extractor can be used for extracting features from any object. The feature
// extractor and feature function classes are template classes that have to
// be instantiated for extracting feature from a specific object type.
//
// A feature extractor consists of a hierarchy of feature functions. Each
// feature function extracts one or more feature type and value pairs from the
// object.
//
// The feature extractor has a modular design where new feature functions can be
// registered as components. The feature extractor is initialized from a
// descriptor represented by a protocol buffer. The feature extractor can also
// be initialized from a text-based source specification of the feature
// extractor. Feature specification parsers can be added as components. By
// default the feature extractor can be read from an ASCII protocol buffer or in
// a simple feature modeling language (fml).

// A feature function is invoked with a focus. Nested feature function can be
// invoked with another focus determined by the parent feature function.

#ifndef LIBTEXTCLASSIFIER_COMMON_FEATURE_EXTRACTOR_H_
#define LIBTEXTCLASSIFIER_COMMON_FEATURE_EXTRACTOR_H_

#include <stddef.h>

#include <string>
#include <vector>

#include "common/feature-descriptors.h"
#include "common/feature-types.h"
#include "common/fml-parser.h"
#include "common/registry.h"
#include "common/task-context.h"
#include "common/workspace.h"
#include "util/base/integral_types.h"
#include "util/base/logging.h"
#include "util/base/macros.h"
#include "util/gtl/stl_util.h"

namespace libtextclassifier {
namespace nlp_core {

typedef int64 Predicate;
typedef Predicate FeatureValue;

// A union used to represent discrete and continuous feature values.
union FloatFeatureValue {
 public:
  explicit FloatFeatureValue(FeatureValue v) : discrete_value(v) {}
  FloatFeatureValue(uint32 i, float w) : id(i), weight(w) {}
  FeatureValue discrete_value;
  struct {
    uint32 id;
    float weight;
  };
};

// A feature vector contains feature type and value pairs.
class FeatureVector {
 public:
  FeatureVector() {}

  // Adds feature type and value pair to feature vector.
  void add(FeatureType *type, FeatureValue value) {
    features_.emplace_back(type, value);
  }

  // Removes all elements from the feature vector.
  void clear() { features_.clear(); }

  // Returns the number of elements in the feature vector.
  int size() const { return features_.size(); }

  // Reserves space in the underlying feature vector.
  void reserve(int n) { features_.reserve(n); }

  // Returns feature type for an element in the feature vector.
  FeatureType *type(int index) const { return features_[index].type; }

  // Returns feature value for an element in the feature vector.
  FeatureValue value(int index) const { return features_[index].value; }

 private:
  // Structure for holding feature type and value pairs.
  struct Element {
    Element() : type(nullptr), value(-1) {}
    Element(FeatureType *t, FeatureValue v) : type(t), value(v) {}

    FeatureType *type;
    FeatureValue value;
  };

  // Array for storing feature vector elements.
  std::vector<Element> features_;

  TC_DISALLOW_COPY_AND_ASSIGN(FeatureVector);
};

// The generic feature extractor is the type-independent part of a feature
// extractor. This holds the descriptor for the feature extractor and the
// collection of feature types used in the feature extractor.  The feature
// types are not available until FeatureExtractor<>::Init() has been called.
class GenericFeatureExtractor {
 public:
  GenericFeatureExtractor();
  virtual ~GenericFeatureExtractor();

  // Initializes the feature extractor from an FML string specification.  For
  // the FML specification grammar, see fml-parser.h.
  //
  // Returns true on success, false on syntax error.
  bool Parse(const std::string &source);

  // Returns the feature extractor descriptor.
  const FeatureExtractorDescriptor &descriptor() const { return descriptor_; }
  FeatureExtractorDescriptor *mutable_descriptor() { return &descriptor_; }

  // Returns the number of feature types in the feature extractor.  Invalid
  // before Init() has been called.
  int feature_types() const { return feature_types_.size(); }

  // Returns a feature type used in the extractor.  Invalid before Init() has
  // been called.
  const FeatureType *feature_type(int index) const {
    return feature_types_[index];
  }

  // Returns the feature domain size of this feature extractor.
  // NOTE: The way that domain size is calculated is, for some, unintuitive. It
  // is the largest domain size of any feature type.
  FeatureValue GetDomainSize() const;

 protected:
  // Initializes the feature types used by the extractor.  Called from
  // FeatureExtractor<>::Init().
  //
  // Returns true on success, false on error.
  bool InitializeFeatureTypes();

 private:
  // Initializes the top-level feature functions.
  virtual bool InitializeFeatureFunctions() = 0;

  // Returns all feature types used by the extractor. The feature types are
  // added to the result array.
  virtual void GetFeatureTypes(std::vector<FeatureType *> *types) const = 0;

  // Descriptor for the feature extractor. This is a protocol buffer that
  // contains all the information about the feature extractor. The feature
  // functions are initialized from the information in the descriptor.
  FeatureExtractorDescriptor descriptor_;

  // All feature types used by the feature extractor. The collection of all the
  // feature types describes the feature space of the feature set produced by
  // the feature extractor.  Not owned.
  std::vector<FeatureType *> feature_types_;

  TC_DISALLOW_COPY_AND_ASSIGN(GenericFeatureExtractor);
};

// The generic feature function is the type-independent part of a feature
// function. Each feature function is associated with the descriptor that it is
// instantiated from.  The feature types associated with this feature function
// will be established by the time FeatureExtractor<>::Init() completes.
class GenericFeatureFunction {
 public:
  // A feature value that represents the absence of a value.
  static constexpr FeatureValue kNone = -1;

  GenericFeatureFunction();
  virtual ~GenericFeatureFunction();

  // Sets up the feature function. NB: FeatureTypes of nested functions are not
  // guaranteed to be available until Init().
  //
  // Returns true on success, false on error.
  virtual bool Setup(TaskContext *context) { return true; }

  // Initializes the feature function. NB: The FeatureType of this function must
  // be established when this method completes.
  //
  // Returns true on success, false on error.
  virtual bool Init(TaskContext *context) { return true; }

  // Requests workspaces from a registry to obtain indices into a WorkspaceSet
  // for any Workspace objects used by this feature function. NB: This will be
  // called after Init(), so it can depend on resources and arguments.
  virtual void RequestWorkspaces(WorkspaceRegistry *registry) {}

  // Appends the feature types produced by the feature function to types.  The
  // default implementation appends feature_type(), if non-null.  Invalid
  // before Init() has been called.
  virtual void GetFeatureTypes(std::vector<FeatureType *> *types) const;

  // Returns the feature type for feature produced by this feature function. If
  // the feature function produces features of different types this returns
  // null.  Invalid before Init() has been called.
  virtual FeatureType *GetFeatureType() const;

  // Returns the name of the registry used for creating the feature function.
  // This can be used for checking if two feature functions are of the same
  // kind.
  virtual const char *RegistryName() const = 0;

  // Returns the value of a named parameter from the feature function
  // descriptor.  Returns empty string ("") if parameter is not found.
  std::string GetParameter(const std::string &name) const;

  // Returns the int value of a named parameter from the feature function
  // descriptor.  Returns default_value if the parameter is not found or if its
  // value can't be parsed as an int.
  int GetIntParameter(const std::string &name, int default_value) const;

  // Returns the bool value of a named parameter from the feature function
  // descriptor.  Returns default_value if the parameter is not found or if its
  // value is not "true" or "false".
  bool GetBoolParameter(const std::string &name, bool default_value) const;

  // Returns the FML function description for the feature function, i.e. the
  // name and parameters without the nested features.
  std::string FunctionName() const {
    std::string output;
    ToFMLFunction(*descriptor_, &output);
    return output;
  }

  // Returns the prefix for nested feature functions. This is the prefix of this
  // feature function concatenated with the feature function name.
  std::string SubPrefix() const {
    return prefix_.empty() ? FunctionName() : prefix_ + "." + FunctionName();
  }

  // Returns/sets the feature extractor this function belongs to.
  GenericFeatureExtractor *extractor() const { return extractor_; }
  void set_extractor(GenericFeatureExtractor *extractor) {
    extractor_ = extractor;
  }

  // Returns/sets the feature function descriptor.
  FeatureFunctionDescriptor *descriptor() const { return descriptor_; }
  void set_descriptor(FeatureFunctionDescriptor *descriptor) {
    descriptor_ = descriptor;
  }

  // Returns a descriptive name for the feature function. The name is taken from
  // the descriptor for the feature function. If the name is empty or the
  // feature function is a variable the name is the FML representation of the
  // feature, including the prefix.
  std::string name() const;

  // Returns the argument from the feature function descriptor. It defaults to
  // 0 if the argument has not been specified.
  int argument() const {
    return descriptor_->has_argument() ? descriptor_->argument() : 0;
  }

  // Returns/sets/clears function name prefix.
  const std::string &prefix() const { return prefix_; }
  void set_prefix(const std::string &prefix) { prefix_ = prefix; }

 protected:
  // Returns the feature type for single-type feature functions.
  FeatureType *feature_type() const { return feature_type_; }

  // Sets the feature type for single-type feature functions.  This takes
  // ownership of feature_type.  Can only be called once with a non-null
  // pointer.
  void set_feature_type(FeatureType *feature_type) {
    TC_DCHECK_NE(feature_type, nullptr);
    feature_type_ = feature_type;
  }

 private:
  // Feature extractor this feature function belongs to.  Not owned.
  GenericFeatureExtractor *extractor_ = nullptr;

  // Descriptor for feature function.  Not owned.
  FeatureFunctionDescriptor *descriptor_ = nullptr;

  // Feature type for features produced by this feature function. If the
  // feature function produces features of multiple feature types this is null
  // and the feature function must return it's feature types in
  // GetFeatureTypes().  Owned.
  FeatureType *feature_type_ = nullptr;

  // Prefix used for sub-feature types of this function.
  std::string prefix_;
};

// Feature function that can extract features from an object.  Templated on
// two type arguments:
//
// OBJ:  The "object" from which features are extracted; e.g., a sentence.  This
//       should be a plain type, rather than a reference or pointer.
//
// ARGS: A set of 0 or more types that are used to "index" into some part of the
//       object that should be extracted, e.g. an int token index for a sentence
//       object.  This should not be a reference type.
template <class OBJ, class... ARGS>
class FeatureFunction
    : public GenericFeatureFunction,
      public RegisterableClass<FeatureFunction<OBJ, ARGS...> > {
 public:
  using Self = FeatureFunction<OBJ, ARGS...>;

  // Preprocesses the object.  This will be called prior to calling Evaluate()
  // or Compute() on that object.
  virtual void Preprocess(WorkspaceSet *workspaces, OBJ *object) const {}

  // Appends features computed from the object and focus to the result.  The
  // default implementation delegates to Compute(), adding a single value if
  // available.  Multi-valued feature functions must override this method.
  virtual void Evaluate(const WorkspaceSet &workspaces, const OBJ &object,
                        ARGS... args, FeatureVector *result) const {
    FeatureValue value = Compute(workspaces, object, args..., result);
    if (value != kNone) result->add(feature_type(), value);
  }

  // Returns a feature value computed from the object and focus, or kNone if no
  // value is computed.  Single-valued feature functions only need to override
  // this method.
  virtual FeatureValue Compute(const WorkspaceSet &workspaces,
                               const OBJ &object, ARGS... args,
                               const FeatureVector *fv) const {
    return kNone;
  }

  // Instantiates a new feature function in a feature extractor from a feature
  // descriptor.
  static Self *Instantiate(GenericFeatureExtractor *extractor,
                           FeatureFunctionDescriptor *fd,
                           const std::string &prefix) {
    Self *f = Self::Create(fd->type());
    if (f != nullptr) {
      f->set_extractor(extractor);
      f->set_descriptor(fd);
      f->set_prefix(prefix);
    }
    return f;
  }

  // Returns the name of the registry for the feature function.
  const char *RegistryName() const override { return Self::registry()->name; }

 private:
  // Special feature function class for resolving variable references. The type
  // of the feature function is used for resolving the variable reference. When
  // evaluated it will either get the feature value(s) from the variable portion
  // of the feature vector, if present, or otherwise it will call the referenced
  // feature extractor function directly to extract the feature(s).
  class Reference;
};

// Base class for features with nested feature functions. The nested functions
// are of type NES, which may be different from the type of the parent function.
// NB: NestedFeatureFunction will ensure that all initialization of nested
// functions takes place during Setup() and Init() -- after the nested features
// are initialized, the parent feature is initialized via SetupNested() and
// InitNested(). Alternatively, a derived classes that overrides Setup() and
// Init() directly should call Parent::Setup(), Parent::Init(), etc. first.
//
// Note: NestedFeatureFunction cannot know how to call Preprocess, Evaluate, or
// Compute, since the nested functions may be of a different type.
template <class NES, class OBJ, class... ARGS>
class NestedFeatureFunction : public FeatureFunction<OBJ, ARGS...> {
 public:
  using Parent = NestedFeatureFunction<NES, OBJ, ARGS...>;

  // Clean up nested functions.
  ~NestedFeatureFunction() override {
    // Fully qualified class name, to avoid an ambiguity error when building for
    // Android.
    ::libtextclassifier::STLDeleteElements(&nested_);
  }

  // By default, just appends the nested feature types.
  void GetFeatureTypes(std::vector<FeatureType *> *types) const override {
    // It's odd if a NestedFeatureFunction does not have anything nested inside
    // it, so we crash in debug mode.  Still, nothing should crash in prod mode.
    TC_DCHECK(!this->nested().empty())
        << "Nested features require nested features to be defined.";
    for (auto *function : nested_) function->GetFeatureTypes(types);
  }

  // Sets up the nested features.
  bool Setup(TaskContext *context) override {
    bool success = CreateNested(this->extractor(), this->descriptor(), &nested_,
                                this->SubPrefix());
    if (!success) {
      return false;
    }
    for (auto *function : nested_) {
      if (!function->Setup(context)) return false;
    }
    if (!SetupNested(context)) {
      return false;
    }
    return true;
  }

  // Sets up this NestedFeatureFunction specifically.
  virtual bool SetupNested(TaskContext *context) { return true; }

  // Initializes the nested features.
  bool Init(TaskContext *context) override {
    for (auto *function : nested_) {
      if (!function->Init(context)) return false;
    }
    if (!InitNested(context)) return false;
    return true;
  }

  // Initializes this NestedFeatureFunction specifically.
  virtual bool InitNested(TaskContext *context) { return true; }

  // Gets all the workspaces needed for the nested functions.
  void RequestWorkspaces(WorkspaceRegistry *registry) override {
    for (auto *function : nested_) function->RequestWorkspaces(registry);
  }

  // Returns the list of nested feature functions.
  const std::vector<NES *> &nested() const { return nested_; }

  // Instantiates nested feature functions for a feature function. Creates and
  // initializes one feature function for each sub-descriptor in the feature
  // descriptor.
  static bool CreateNested(GenericFeatureExtractor *extractor,
                           FeatureFunctionDescriptor *fd,
                           std::vector<NES *> *functions,
                           const std::string &prefix) {
    for (int i = 0; i < fd->feature_size(); ++i) {
      FeatureFunctionDescriptor *sub = fd->mutable_feature(i);
      NES *f = NES::Instantiate(extractor, sub, prefix);
      if (f == nullptr) {
        return false;
      }
      functions->push_back(f);
    }
    return true;
  }

 protected:
  // The nested feature functions, if any, in order of declaration in the
  // feature descriptor.  Owned.
  std::vector<NES *> nested_;
};

// Base class for a nested feature function that takes nested features with the
// same signature as these features, i.e. a meta feature. For this class, we can
// provide preprocessing of the nested features.
template <class OBJ, class... ARGS>
class MetaFeatureFunction
    : public NestedFeatureFunction<FeatureFunction<OBJ, ARGS...>, OBJ,
                                   ARGS...> {
 public:
  // Preprocesses using the nested features.
  void Preprocess(WorkspaceSet *workspaces, OBJ *object) const override {
    for (auto *function : this->nested_) {
      function->Preprocess(workspaces, object);
    }
  }
};

// Template for a special type of locator: The locator of type
// FeatureFunction<OBJ, ARGS...> calls nested functions of type
// FeatureFunction<OBJ, IDX, ARGS...>, where the derived class DER is
// responsible for translating by providing the following:
//
// // Gets the new additional focus.
// IDX GetFocus(const WorkspaceSet &workspaces, const OBJ &object);
//
// This is useful to e.g. add a token focus to a parser state based on some
// desired property of that state.
template <class DER, class OBJ, class IDX, class... ARGS>
class FeatureAddFocusLocator
    : public NestedFeatureFunction<FeatureFunction<OBJ, IDX, ARGS...>, OBJ,
                                   ARGS...> {
 public:
  void Preprocess(WorkspaceSet *workspaces, OBJ *object) const override {
    for (auto *function : this->nested_) {
      function->Preprocess(workspaces, object);
    }
  }

  void Evaluate(const WorkspaceSet &workspaces, const OBJ &object, ARGS... args,
                FeatureVector *result) const override {
    IDX focus =
        static_cast<const DER *>(this)->GetFocus(workspaces, object, args...);
    for (auto *function : this->nested()) {
      function->Evaluate(workspaces, object, focus, args..., result);
    }
  }

  // Returns the first nested feature's computed value.
  FeatureValue Compute(const WorkspaceSet &workspaces, const OBJ &object,
                       ARGS... args,
                       const FeatureVector *result) const override {
    IDX focus =
        static_cast<const DER *>(this)->GetFocus(workspaces, object, args...);
    return this->nested()[0]->Compute(workspaces, object, focus, args...,
                                      result);
  }
};

// CRTP feature locator class. This is a meta feature that modifies ARGS and
// then calls the nested feature functions with the modified ARGS. Note that in
// order for this template to work correctly, all of ARGS must be types for
// which the reference operator & can be interpreted as a pointer to the
// argument. The derived class DER must implement the UpdateFocus method which
// takes pointers to the ARGS arguments:
//
// // Updates the current arguments.
// void UpdateArgs(const OBJ &object, ARGS *...args) const;
template <class DER, class OBJ, class... ARGS>
class FeatureLocator : public MetaFeatureFunction<OBJ, ARGS...> {
 public:
  // Feature locators have an additional check that there is no intrinsic type,
  // but only in debug mode: having an intrinsic type here is odd, but not
  // enough to motive a crash in prod.
  void GetFeatureTypes(std::vector<FeatureType *> *types) const override {
    TC_DCHECK_EQ(this->feature_type(), nullptr)
        << "FeatureLocators should not have an intrinsic type.";
    MetaFeatureFunction<OBJ, ARGS...>::GetFeatureTypes(types);
  }

  // Evaluates the locator.
  void Evaluate(const WorkspaceSet &workspaces, const OBJ &object, ARGS... args,
                FeatureVector *result) const override {
    static_cast<const DER *>(this)->UpdateArgs(workspaces, object, &args...);
    for (auto *function : this->nested()) {
      function->Evaluate(workspaces, object, args..., result);
    }
  }

  // Returns the first nested feature's computed value.
  FeatureValue Compute(const WorkspaceSet &workspaces, const OBJ &object,
                       ARGS... args,
                       const FeatureVector *result) const override {
    static_cast<const DER *>(this)->UpdateArgs(workspaces, object, &args...);
    return this->nested()[0]->Compute(workspaces, object, args..., result);
  }
};

// Feature extractor for extracting features from objects of a certain class.
// Template type parameters are as defined for FeatureFunction.
template <class OBJ, class... ARGS>
class FeatureExtractor : public GenericFeatureExtractor {
 public:
  // Feature function type for top-level functions in the feature extractor.
  typedef FeatureFunction<OBJ, ARGS...> Function;
  typedef FeatureExtractor<OBJ, ARGS...> Self;

  // Feature locator type for the feature extractor.
  template <class DER>
  using Locator = FeatureLocator<DER, OBJ, ARGS...>;

  // Initializes feature extractor.
  FeatureExtractor() {}

  ~FeatureExtractor() override {
    // Fully qualified class name, to avoid an ambiguity error when building for
    // Android.
    ::libtextclassifier::STLDeleteElements(&functions_);
  }

  // Sets up the feature extractor. Note that only top-level functions exist
  // until Setup() is called. This does not take ownership over the context,
  // which must outlive this.
  bool Setup(TaskContext *context) {
    for (Function *function : functions_) {
      if (!function->Setup(context)) return false;
    }
    return true;
  }

  // Initializes the feature extractor.  Must be called after Setup().  This
  // does not take ownership over the context, which must outlive this.
  bool Init(TaskContext *context) {
    for (Function *function : functions_) {
      if (!function->Init(context)) return false;
    }
    if (!this->InitializeFeatureTypes()) {
      return false;
    }
    return true;
  }

  // Requests workspaces from the registry. Must be called after Init(), and
  // before Preprocess(). Does not take ownership over registry. This should be
  // the same registry used to initialize the WorkspaceSet used in Preprocess()
  // and ExtractFeatures(). NB: This is a different ordering from that used in
  // SentenceFeatureRepresentation style feature computation.
  void RequestWorkspaces(WorkspaceRegistry *registry) {
    for (auto *function : functions_) function->RequestWorkspaces(registry);
  }

  // Preprocesses the object using feature functions for the phase.  Must be
  // called before any calls to ExtractFeatures() on that object and phase.
  void Preprocess(WorkspaceSet *workspaces, OBJ *object) const {
    for (Function *function : functions_) {
      function->Preprocess(workspaces, object);
    }
  }

  // Extracts features from an object with a focus. This invokes all the
  // top-level feature functions in the feature extractor. Only feature
  // functions belonging to the specified phase are invoked.
  void ExtractFeatures(const WorkspaceSet &workspaces, const OBJ &object,
                       ARGS... args, FeatureVector *result) const {
    result->reserve(this->feature_types());

    // Extract features.
    for (int i = 0; i < functions_.size(); ++i) {
      functions_[i]->Evaluate(workspaces, object, args..., result);
    }
  }

 private:
  // Creates and initializes all feature functions in the feature extractor.
  bool InitializeFeatureFunctions() override {
    // Create all top-level feature functions.
    for (int i = 0; i < descriptor().feature_size(); ++i) {
      FeatureFunctionDescriptor *fd = mutable_descriptor()->mutable_feature(i);
      Function *function = Function::Instantiate(this, fd, "");
      if (function == nullptr) return false;
      functions_.push_back(function);
    }
    return true;
  }

  // Collect all feature types used in the feature extractor.
  void GetFeatureTypes(std::vector<FeatureType *> *types) const override {
    for (Function *function : functions_) {
      function->GetFeatureTypes(types);
    }
  }

  // Top-level feature functions (and variables) in the feature extractor.
  // Owned.  INVARIANT: contains only non-null pointers.
  std::vector<Function *> functions_;
};

#define REGISTER_FEATURE_FUNCTION(base, name, component) \
  REGISTER_CLASS_COMPONENT(base, name, component)

}  // namespace nlp_core
}  // namespace libtextclassifier

#endif  // LIBTEXTCLASSIFIER_COMMON_FEATURE_EXTRACTOR_H_
