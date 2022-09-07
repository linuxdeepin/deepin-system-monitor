// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef __ADDR_PRI_H__
#define __ADDR_PRI_H__


#include <utility>
#include <type_traits>



//base on C++11

/**********************************************************
             access private function
**********************************************************/


namespace std {
  template <bool B, class T = void>
  using enable_if_t = typename enable_if<B, T>::type;
  template <class T>
  using remove_reference_t = typename remove_reference<T>::type;
} // std

// Unnamed namespace is used to avoid duplicate symbols if the macros are used
namespace {
  namespace private_access_detail {

    // @tparam TagType, used to declare different "get" funciton overloads for
    // different members/statics
    template <typename PtrType, PtrType PtrValue, typename TagType>
    struct private_access {
      // Normal lookup cannot find in-class defined (inline) friend functions.
      friend PtrType get(TagType) { return PtrValue; }
    };

  } // namespace private_access_detail
} // namespace

// Used macro naming conventions:
// The "namespace" of this macro library is PRIVATE_ACCESS, i.e. all
// macro here has this prefix.
// All implementation macro, which are not meant to be used directly have the
// PRIVATE_ACCESS_DETAIL prefix.
// Some macros have the ABCD_IMPL form, which means they contain the
// implementation details for the specific ABCD macro.

#define PRIVATE_ACCESS_DETAIL_CONCATENATE_IMPL(x, y) x##y
#define PRIVATE_ACCESS_DETAIL_CONCATENATE(x, y)                                \
  PRIVATE_ACCESS_DETAIL_CONCATENATE_IMPL(x, y)

// @param PtrTypeKind E.g if we have "class A", then it can be "A::*" in case of
// members, or it can be "*" in case of statics.
#define PRIVATE_ACCESS_DETAIL_ACCESS_PRIVATE(Tag, Class, Type, Name,           \
                                             PtrTypeKind)                      \
  namespace {                                                                  \
    namespace private_access_detail {                                          \
      /* Tag type, used to declare different get funcitons for different       \
       * members                                                               \
       */                                                                      \
      struct Tag {};                                                           \
      /* Explicit instantiation */                                             \
      template struct private_access<decltype(&Class::Name), &Class::Name,     \
                                     Tag>;                                     \
      /* We can build the PtrType only with two aliases */                     \
      /* E.g. using PtrType = int(int) *; would be illformed */                \
      using PRIVATE_ACCESS_DETAIL_CONCATENATE(Alias_, Tag) = Type;             \
      using PRIVATE_ACCESS_DETAIL_CONCATENATE(PtrType_, Tag) =                 \
          PRIVATE_ACCESS_DETAIL_CONCATENATE(Alias_, Tag) PtrTypeKind;          \
      /* Declare the friend function, now it is visible in namespace scope.    \
       * Note,                                                                 \
       * we could declare it inside the Tag type too, in that case ADL would   \
       * find                                                                  \
       * the declaration. By choosing to declare it here, the Tag type remains \
       * a                                                                     \
       * simple tag type, it has no other responsibilities. */                 \
      PRIVATE_ACCESS_DETAIL_CONCATENATE(PtrType_, Tag) get(Tag);               \
    }                                                                          \
  }

#define PRIVATE_ACCESS_DETAIL_ACCESS_PRIVATE_FIELD(Tag, Class, Type, Name)     \
  PRIVATE_ACCESS_DETAIL_ACCESS_PRIVATE(Tag, Class, Type, Name, Class::*)       \
  namespace {                                                                  \
    namespace access_private_field {                                                  \
      Type &Class##Name(Class &&t) { return t.*get(private_access_detail::Tag{}); }   \
      Type &Class##Name(Class &t) { return t.*get(private_access_detail::Tag{}); }    \
      /* The following usings are here to avoid duplicate const qualifier      \
       * warnings                                                              \
       */                                                                      \
      using PRIVATE_ACCESS_DETAIL_CONCATENATE(X, Tag) = Type;                  \
      using PRIVATE_ACCESS_DETAIL_CONCATENATE(Y, Tag) =                        \
          const PRIVATE_ACCESS_DETAIL_CONCATENATE(X, Tag);                     \
      PRIVATE_ACCESS_DETAIL_CONCATENATE(Y, Tag) & Class##Name(const Class &t) {\
        return t.*get(private_access_detail::Tag{});                           \
      }                                                                        \
    }                                                                          \
  }

#define PRIVATE_ACCESS_DETAIL_ACCESS_PRIVATE_FUN(Tag, Class, Type, Name)       \
  PRIVATE_ACCESS_DETAIL_ACCESS_PRIVATE(Tag, Class, Type, Name, Class::*)       \
  namespace {                                                                  \
    namespace call_private_fun {                                               \
      /* We do perfect forwarding, but we want to restrict the overload set    \
       * only for objects which have the type Class. */                        \
      template <typename Obj,                                                  \
                std::enable_if_t<std::is_same<std::remove_reference_t<Obj>,    \
                                              Class>::value> * = nullptr,      \
                typename... Args>                                              \
      auto Class##Name(Obj &&o, Args &&... args) -> decltype(                  \
          (std::forward<Obj>(o).*                                              \
           get(private_access_detail::Tag{}))(std::forward<Args>(args)...)) {  \
        return (std::forward<Obj>(o).*get(private_access_detail::Tag{}))(      \
            std::forward<Args>(args)...);                                      \
      }                                                                        \
    }                                                                          \
    namespace get_private_fun {                                                \
      auto Class##Name()  -> decltype(                                         \
          get(private_access_detail::Tag{})) {                                 \
        return (get(private_access_detail::Tag{}));                            \
      }                                                                        \
    }                                                                          \
  }

#define PRIVATE_ACCESS_DETAIL_ACCESS_PRIVATE_STATIC_FIELD(Tag, Class, Type,    \
                                                          Name)                \
  PRIVATE_ACCESS_DETAIL_ACCESS_PRIVATE(Tag, Class, Type, Name, *)              \
  namespace {                                                                  \
    namespace access_private_static_field {                                    \
      namespace Class {                                                        \
        Type &Class##Name() { return *get(private_access_detail::Tag{}); }     \
      }                                                                        \
    }                                                                          \
  }

#define PRIVATE_ACCESS_DETAIL_ACCESS_PRIVATE_STATIC_FUN(Tag, Class, Type,      \
                                                        Name)                  \
  PRIVATE_ACCESS_DETAIL_ACCESS_PRIVATE(Tag, Class, Type, Name, *)              \
  namespace {                                                                  \
    namespace call_private_static_fun {                                        \
      namespace Class {                                                        \
        template <typename... Args>                                            \
        auto Class##Name(Args &&... args) -> decltype(                         \
            get(private_access_detail::Tag{})(std::forward<Args>(args)...)) {  \
          return get(private_access_detail::Tag{})(                            \
              std::forward<Args>(args)...);                                    \
        }                                                                      \
      }                                                                        \
    }                                                                          \
    namespace get_private_static_fun {                                         \
      namespace Class {                                                        \
        auto Class##Name() -> decltype(get(private_access_detail::Tag{})) {    \
          return get(private_access_detail::Tag{});                            \
        }                                                                      \
      }                                                                        \
    }                                                                          \
  }

#define PRIVATE_ACCESS_DETAIL_UNIQUE_TAG                                       \
  PRIVATE_ACCESS_DETAIL_CONCATENATE(PrivateAccessTag, __COUNTER__)

#define ACCESS_PRIVATE_FIELD(Class, Type, Name)                                \
  PRIVATE_ACCESS_DETAIL_ACCESS_PRIVATE_FIELD(PRIVATE_ACCESS_DETAIL_UNIQUE_TAG, \
                                             Class, Type, Name)

#define ACCESS_PRIVATE_FUN(Class, Type, Name)                                  \
  PRIVATE_ACCESS_DETAIL_ACCESS_PRIVATE_FUN(PRIVATE_ACCESS_DETAIL_UNIQUE_TAG,   \
                                           Class, Type, Name)

#define ACCESS_PRIVATE_STATIC_FIELD(Class, Type, Name)                         \
    Type Class::Name;                                                          \
  PRIVATE_ACCESS_DETAIL_ACCESS_PRIVATE_STATIC_FIELD(                           \
      PRIVATE_ACCESS_DETAIL_UNIQUE_TAG, Class, Type, Name)

#define ACCESS_PRIVATE_STATIC_FUN(Class, Type, Name)                           \
  PRIVATE_ACCESS_DETAIL_ACCESS_PRIVATE_STATIC_FUN(                             \
      PRIVATE_ACCESS_DETAIL_UNIQUE_TAG, Class, Type, Name)

#endif