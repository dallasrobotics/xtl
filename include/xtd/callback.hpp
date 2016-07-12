/** @file
Single producer notifies multiple receivers of an event. Callbacks are performed serially and the order is undefined when multiple receivers are attached
@copyright David Mott (c) 2016. Distributed under the Boost Software License Version 1.0. See LICENSE.md or http://boost.org/LICENSE_1_0.txt for details.
 @example example_callback.cpp
*/

#pragma once

namespace xtd{
  /** @{
  Generic callback declaration
  @tparam _FnSig Function signature of the callback. Attached receivers must match the signature.
  */
  template <typename _FnSig> class callback;

  /** Specialization for a callback that has a return value
  This specialization should used with care when multiple receivers are attached because only a single return value is appropriate.
  @tparam _ReturnT the return type
  @tparam ... variadic list of arguments
   */
  template <typename _ReturnT, typename ... _Args> class callback < _ReturnT(_Args...) >{
    class invoker{
    public:
      using ptr = std::unique_ptr<invoker>;
      using vector = std::vector<ptr>;
      virtual ~invoker() = default;
      virtual _ReturnT invoke(_Args...) const = 0;
    };

    template <typename _MethodT, _MethodT * _method> class method_invoker : public invoker{
    public:
      ~method_invoker() override = default;
      virtual _ReturnT invoke(_Args...oArgs) const override { return (*_method)(std::forward<_Args>(oArgs)...); }
    };

    template <typename _LambdaT> class lamdba_invoker : public invoker{
    public:
      ~lamdba_invoker() override = default;
      virtual _ReturnT invoke(_Args...oArgs) const override { return _Lambda(std::forward<_Args>(oArgs)...); }
      explicit lamdba_invoker(_LambdaT oLambda) : _Lambda(oLambda){} //NOSONAR
      lamdba_invoker(lamdba_invoker&& src) : _Lambda(std::move(src._Lambda)){} //NOSONAR
      lamdba_invoker(const lamdba_invoker& src) : _Lambda(src._Lambda){} //NOSONAR
      lamdba_invoker& operator=(const lamdba_invoker& src){
        if (this != &src){
          _Lambda = src._Lambda;
        }
        return *this;
      }
      lamdba_invoker& operator=(lamdba_invoker&&) = delete;
      _LambdaT _Lambda;
    };

    template <typename _DestT, _ReturnT(_DestT::*_member)(_Args...)> class member_invoker : public invoker{
    public:
      ~member_invoker() override = default;
      member_invoker() = delete;
      member_invoker& operator=(const member_invoker&) = delete;
      member_invoker(member_invoker&& oSrc) : _dest(oSrc._dest){}  //NOSONAR
      member_invoker(const member_invoker& oSrc) : _dest(oSrc._dest){}  //NOSONAR
      explicit member_invoker(_DestT* dest) : _dest(dest){} //NOSONAR
      virtual _ReturnT invoke(_Args...oArgs) const override { return (_dest->*_member)(oArgs...); }
      _DestT * _dest;
    };

    typename invoker::vector _Invokers;

  public:

    enum result_policy{
      return_first,
      return_last
    };

    ~callback() = default;
    callback() = default;
    callback(const callback&) = delete;
    callback(callback&& src) : _Invokers(std::move(src._Invokers)){}
    callback& operator=(const callback&) = delete;

    _ReturnT operator()(result_policy result, _Args...oArgs) const{
      _ReturnT oRet;
      typename invoker::vector::size_type i = 0;
      for (const auto & oInvoker : _Invokers) {
        if ((result_policy::first == result && 0==i) || (result_policy::last == result && (_Invokers.size()-1) == i)) {
          oRet = oInvoker->invoke(std::forward<_Args>(oArgs)...);
        }else{
          oInvoker->invoke(oArgs...);
        }
        ++i;
      }
      return oRet;
    }

    template <typename _ClassT, _ReturnT(_ClassT::*_MemberT)(_Args...)>
    void connect(_ClassT *pClass){ _Invokers.emplace_back(new member_invoker<_ClassT, _MemberT>(pClass)); }

    template <typename method>
    void connect(method oMethod){ _Invokers.emplace_back(new lamdba_invoker< method >(oMethod)); }

    template <_ReturnT(*method)(_Args...)>
    void connect(){ _Invokers.emplace_back(new method_invoker<_ReturnT(_Args...), method>()); }

  };

  /** Specialziation for a callback with a void return value
  @tparam ... variadic list of arguments
  */
  template <typename ... _Args> class callback < void(_Args...) >{

    using _ReturnT = void;

    class invoker{
    public:
      using ptr = std::unique_ptr<invoker>;
      using vector = std::vector<ptr>;
      virtual ~invoker() = default;
      virtual _ReturnT invoke(_Args...) const = 0;
    };

    template <typename _MethodT, _MethodT * _method> class method_invoker : public invoker{
    public:
      ~method_invoker() override = default;
      virtual _ReturnT invoke(_Args...oArgs) const override { (*_method)(std::forward<_Args>(oArgs)...); }
    };

    template <typename _LambdaT> class lamdba_invoker : public invoker{
    public:
      ~lamdba_invoker() override = default;
      virtual _ReturnT invoke(_Args...oArgs) const override { _Lambda(std::forward<_Args>(oArgs)...); }
      explicit lamdba_invoker(_LambdaT oLambda) : _Lambda(oLambda){}
      lamdba_invoker(lamdba_invoker&& src) : _Lambda(std::move(src._Lambda)){}
      lamdba_invoker(const lamdba_invoker& src) : _Lambda(src._Lambda){}
      lamdba_invoker& operator=(const lamdba_invoker& src){
        if (this != &src){
          _Lambda = src._Lambda;
        }
        return *this;
      }
      lamdba_invoker& operator=(lamdba_invoker&&) = delete;
      _LambdaT _Lambda;
    };

    template <typename _DestT, _ReturnT(_DestT::*_member)(_Args...)> class member_invoker : public invoker{
    public:
      ~member_invoker() override = default;
      member_invoker() = delete;
      member_invoker& operator=(const member_invoker&) = delete;
      member_invoker(member_invoker&& oSrc) : _dest(oSrc._dest){}
      member_invoker(const member_invoker& oSrc) : _dest(oSrc._dest){}
      explicit member_invoker(_DestT* dest) : _dest(dest){}
      virtual _ReturnT invoke(_Args...oArgs)const override { (_dest->*_member)(oArgs...); }
      _DestT * _dest;
    };

    typename invoker::vector _Invokers;

  public:
    ~callback() = default;
    callback() = default;
    callback(const callback&) = delete;
    callback(callback&& src) : _Invokers(std::move(src._Invokers)){}
    callback& operator=(const callback&) = delete;


    void operator()(_Args...oArgs) const{
      for (auto & oInvoker : _Invokers){
        oInvoker->invoke(oArgs...);
      }
    }

    template <typename _ClassT, _ReturnT(_ClassT::*_MemberT)(_Args...)>
    void connect(_ClassT *pClass){ _Invokers.emplace_back(new member_invoker<_ClassT, _MemberT>(pClass)); }

    template <typename method>
    void connect(method oMethod){ _Invokers.emplace_back(new lamdba_invoker< method >(oMethod)); }

    template <_ReturnT(*method)(_Args...)>
    void connect(){ _Invokers.emplace_back(new method_invoker<void(_Args...), method>()); }

    template <typename _Ty>
    callback& operator += (_Ty&& addend){ connect(std::forward<_Ty>(addend)); return *this; }


  };
  ///@}
}
