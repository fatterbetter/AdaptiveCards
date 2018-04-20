// Copyright (c) Microsoft Corporation. All rights reserved.
//
// Licensed under the MIT License. See LICENSE.txt in the project root for license information.

#pragma once

#include "winstring.h"

//
// Helpers for error handling.  General strategy is that errors are always
// reported as exceptions (so we don't need to special case any STL usage).
//
// Methods that need to return HRESULTs wrap their body inside an
// ExceptionBoundary.
//


//
// Generally all errors are reported with an associated HRESULT.  These are
// represented as an HResultException.  Use ExceptionBoundary() to catch
// exceptions and translate them to HRESULTs.
//
// As the constructor is protected, only ThrowHR() can be used to throw an
// HResultException.  This provides future flexibility in case we some day
// want to throw more varied types of exception.
//
class HResultException
{
    HRESULT m_Hr;

protected:
    explicit HResultException(HRESULT hr)
        : m_Hr(hr)
    {}

public:
    HRESULT GetHr() const
    {
        return m_Hr;
    }

    __declspec(noreturn)
        friend void ThrowHR(HRESULT);
};

//
// Throws an exception for the given HRESULT.
//
__declspec(noreturn) __declspec(noinline)
inline void ThrowHR(HRESULT hr)
{
    throw HResultException(hr);
}

//
// Throws the appropriate exception for the given HRESULT, attaching a custom error message
// string. To avoid leaks, the message string should be owned by an RAII wrapper such as
// WinString. We don't take this parameter directly as a WinString to break what would
// otherwise be a circular dependency (WinString uses ErrorHandling.h in its implementation).
//
__declspec(noreturn) __declspec(noinline)
inline void ThrowHR(HRESULT hr, HSTRING message)
{
    RoOriginateError(hr, message);

    ThrowHR(hr);
}

__declspec(noreturn) __declspec(noinline)
inline void ThrowHR(HRESULT hr, wchar_t const* message)
{
    using ::Microsoft::WRL::Wrappers::HStringReference;

    ThrowHR(hr, HStringReference(message).Get());
}

//
// Throws if the HR fails.  This is the workhorse helper for converting calls to
// functions that return HRESULTs to exceptions. eg:
//
//    ThrowIfFailed(myObj->MethodThatReturnsHRESULT());
//
inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
        ThrowHR(hr);
}

//
// Throws if the given pointer is null.
//
template<typename T>
inline void ThrowIfNullPointer(T* ptr, HRESULT hrToThrow)
{
    if (ptr == nullptr)
        ThrowHR(hrToThrow);
}

template<typename T>
inline void ThrowIfNegative(T value)
{
    if (value < 0)
        ThrowHR(E_INVALIDARG);
}

inline void ThrowIfZeroOrNegative(uint32_t n)
{
    if (n <= 0)
    {
        ThrowHR(E_INVALIDARG);
    }
}

//
// Checks that a given pointer argument is valid (ie non-null).  This is
// expected to be used at the beginning of methods to validate pointer
// parameters that are marked as [in].
//
template<typename T>
inline void CheckInPointer(T* ptr)
{
    ThrowIfNullPointer(ptr, E_INVALIDARG);
}

//
// Checks that a given output pointer parameter is valid (ie non-null), and sets
// it to null.  This is expected to be used at the beginning of methods to
// validate out pointer parameters that are marked as [out].
//
template<typename T>
inline void CheckAndClearOutPointer(T** ptr)
{
    CheckInPointer(ptr);
    *ptr = nullptr;
}

//
// WRL's Make<>() function returns an empty ComPtr on failure rather than
// throwing an exception.  This checks the result and throws bad_alloc.
//
// Note: generally we use exceptions inside constructors to report errors.
// Therefore the only way that Make() will return an error is if an allocation
// fails.
//
__declspec(noreturn) __declspec(noinline)
inline void ThrowBadAlloc()
{
    throw std::bad_alloc();
}

inline void CheckMakeResult(bool result)
{
    if (!result)
        ThrowBadAlloc();
}

//
// Converts exceptions in the callable code into HRESULTs.
//
__declspec(noinline)
inline HRESULT ThrownExceptionToHResult()
{
    try
    {
        throw;
    }
    catch (HResultException const& e)
    {
        return e.GetHr();
    }
    catch (std::bad_alloc const&)
    {
        return E_OUTOFMEMORY;
    }
    catch (...)
    {
        return E_UNEXPECTED;
    }
}

template<typename CALLABLE>
HRESULT ExceptionBoundary(CALLABLE&& fn)
{
    try
    {
        fn();
        return S_OK;
    }
    catch (...)
    {
        return ThrownExceptionToHResult();
    }
}

// Copyright (c) Microsoft Corporation. All rights reserved.
//
// Licensed under the MIT License. See LICENSE.txt in the project root for license information.

#pragma once

#include <algorithm>
#include <assert.h>
#include <iterator>
#include <mutex>
#include <typeindex>
#include <unordered_map>

// By default, object lifespan tracking is only enabled in debug builds.
// Define ENABLE_LIFESPAN_TRACKER or DISABLE_LIFESPAN_TRACKER to force it on or off.
#if defined _DEBUG && !defined DISABLE_LIFESPAN_TRACKER
#define ENABLE_LIFESPAN_TRACKER
#endif


#ifdef ENABLE_LIFESPAN_TRACKER


// Specialize these to (temporarily!) enable more intrusive lifespan reporting for specific types.
template<typename T> bool TraceAllocations() { return false; }
template<typename T> bool BreakOnAllocation() { return false; }


// Derive from this to make a type trackable.
template<typename T>
class LifespanTracker
{
protected:
    LifespanTracker()
    {
        LifespanInfo::AddObject(typeid(T), TraceAllocations<T>(), BreakOnAllocation<T>());
    }

    ~LifespanTracker()
    {
        LifespanInfo::RemoveObject(typeid(T), TraceAllocations<T>(), BreakOnAllocation<T>());
    }
};


class LifespanInfo
{
    struct State
    {
        std::mutex mutex;
        std::unordered_map<std::type_index, size_t> objectCounts;
    };

    static State m_state;

    LifespanInfo() = delete;


public:
    // Records that a new object is being allocated.
    static void AddObject(type_info const& type, bool traceAllocations, bool breakOnAllocation)
    {
        std::lock_guard<std::mutex> lock(m_state.mutex);

        size_t objectCount = ++m_state.objectCounts[type];

        TraceAllocation(type, traceAllocations, breakOnAllocation, objectCount, true);
    }


    // Records that an object is being freed.
    static void RemoveObject(type_info const& type, bool traceAllocations, bool breakOnAllocation)
    {
        std::lock_guard<std::mutex> lock(m_state.mutex);

        size_t objectCount = --m_state.objectCounts[type];

        TraceAllocation(type, traceAllocations, breakOnAllocation, objectCount, false);
    }


    // Debug spews the counts of all live objects, and returns the total count.
    static size_t ReportLiveObjects()
    {
        std::lock_guard<std::mutex> lock(m_state.mutex);

        return ReportLiveObjectsNoLock();
    }


    // Debug spews the counts of all live objects, without taking out the usual lock.
    // For use during DLL unload where synchronization primitives are no longer available.
    static size_t ReportLiveObjectsNoLock()
    {
        typedef std::pair<std::type_index, size_t> pair_t;

        // Filter out only types that have remaining live instances.
        std::vector<pair_t> liveObjects;

        std::copy_if(m_state.objectCounts.begin(), m_state.objectCounts.end(),
            back_inserter(liveObjects),
            [](pair_t const& typeAndCount) { return typeAndCount.second > 0; });

        // Sort by live instance count (highest first).
        std::sort(liveObjects.begin(), liveObjects.end(),
            [](pair_t const& a, pair_t const& b)
        {
            return (a.second != b.second) ? (a.second > b.second)
                : (strcmp(a.first.name(), b.first.name()) < 0);
        });

        // Output the results.
        size_t totalLiveObjects = 0;

        if (!liveObjects.empty())
        {
            OutputDebugString(L"Live Win2D objects (may indicate leaks):\n");

            for (auto& typeAndCount : liveObjects)
            {
                totalLiveObjects += typeAndCount.second;
            }
        }

        return totalLiveObjects;
    }


    // Wipes all lifespan data. This is used by unit tests to start each test invocation with
    // a clean state, so leaks in one test don't also cause failure reports from subsequent ones.
    static void Reset()
    {
        std::lock_guard<std::mutex> lock(m_state.mutex);

        m_state.objectCounts.clear();
    }


private:
    static void TraceAllocation(type_info const& type, bool traceAllocations, bool breakOnAllocation, size_t objectCount, bool isNew)
    {
        // Optional debug break for alloc and free of selected types.
        if (breakOnAllocation)
        {
            __debugbreak();
        }
    }
};


__declspec(selectany) LifespanInfo::State LifespanInfo::m_state;


#else   // ENABLE_LIFESPAN_TRACKER


// No-op implementation for when lifespan tracking is disabled.
template<typename T>
struct LifespanTracker { };

struct LifespanInfo
{
    static size_t ReportLiveObjects() { return 0; }
    static size_t ReportLiveObjectsNoLock() { return 0; }

    static void Reset() { }
};


#endif  // ENABLE_LIFESPAN_TRACKER

// Copyright (c) Microsoft Corporation. All rights reserved.
//
// Licensed under the MIT License. See LICENSE.txt in the project root for license information.

namespace borrowed
{
    // Element traits describe how to store and manipulate the values inside a collection.
    // This default implementation is for value types. The same template is specialized with
    // more interesting versions for reference counted pointer types and strings.
    template<typename T>
    struct ElementTraits
    {
        typedef T ElementType;

        static ElementType Wrap(T const& value)
        {
            return value;
        }

        static void Unwrap(ElementType const& value, _Out_ T* result)
        {
            *result = value;
        }

        static bool Equals(T const& value1, T const& value2)
        {
            return value1 == value2;
        }
    };


    // Specialized element traits for reference counted pointer types.
    template<typename T>
    struct ElementTraits<T*>
    {
        typedef Microsoft::WRL::ComPtr<T> ElementType;

        static ElementType Wrap(T* value)
        {
            return Microsoft::WRL::ComPtr<T>(value);
        }

        static void Unwrap(ElementType const& value, _Out_ T** result)
        {
            ThrowIfFailed(value.CopyTo(result));
        }

        static bool Equals(Microsoft::WRL::ComPtr<T> const& value1, T* value2)
        {
            return value1.Get() == value2;
        }
    };


    // Specialized element traits for strings.
    template<>
    struct ElementTraits<HSTRING>
    {
        typedef HString ElementType;

        static ElementType Wrap(HSTRING const& value)
        {
            HString hstringValue;
            hstringValue.Set(value);
            return hstringValue;
        }

        static void Unwrap(ElementType const& value, _Out_ HSTRING* result)
        {
            value.CopyTo(result);
        }

        static bool Equals(HSTRING const& value1, HSTRING const& value2)
        {
            int compareResult;
            ThrowIfFailed(WindowsCompareStringOrdinal(value1, value2, &compareResult));
            return compareResult == 0;
        }
    };


    // Vector traits describe how the collection itself is implemented.
    // This default version just uses an STL vector.
    template<typename T>
    struct DefaultVectorTraits : public ElementTraits<T>
    {
        typedef std::vector<ElementType> InternalVectorType;


        static unsigned GetSize(InternalVectorType const& vector)
        {
            return (unsigned)vector.size();
        };


        static ElementType GetAt(InternalVectorType const& vector, unsigned index)
        {
            if (index >= vector.size())
                ThrowHR(E_BOUNDS);

            return vector[index];
        }


        static void SetAt(InternalVectorType& vector, unsigned index, T const& item)
        {
            if (index >= vector.size())
                ThrowHR(E_BOUNDS);

            vector[index] = Wrap(item);
        }


        static void InsertAt(InternalVectorType& vector, unsigned index, T const& item)
        {
            if (index > vector.size())
                ThrowHR(E_BOUNDS);

            vector.insert(vector.begin() + index, Wrap(item));
        }


        static void RemoveAt(InternalVectorType& vector, unsigned index)
        {
            if (index >= vector.size())
                ThrowHR(E_BOUNDS);

            vector.erase(vector.begin() + index);
        }


        static void Append(InternalVectorType& vector, T const& item)
        {
            vector.push_back(Wrap(item));
        }


        static void Clear(InternalVectorType& vector)
        {
            vector.clear();
        }
    };


    // Implements the WinRT IVector interface.
    template<typename T, template<typename T_abi> class Traits = DefaultVectorTraits>
    class Vector : public Microsoft::WRL::RuntimeClass<ABI::Windows::Foundation::Collections::IVector<T>,
        ABI::Windows::Foundation::Collections::IIterable<T>>,
        private LifespanTracker<Vector<T, Traits>>
    {
        InspectableClass(IVector<T>::z_get_rc_name_impl(), BaseTrust);

    public:
        // T_abi is often the same as T, but if T is a runtime class, T_abi will be the corresponding interface.
        typedef typename ABI::Windows::Foundation::Internal::GetAbiType<typename RuntimeClass::IVector::T_complex>::type T_abi;

        // Specialize our traits class to use the ABI version of the type.
        typedef Traits<T_abi> Traits;

    private:
        // Fields.
        typename Traits::InternalVectorType mVector;

        bool isFixedSize;
        bool isChanged;


    public:
        // Constructs an empty vector.
        Vector()
            : isFixedSize(false),
            isChanged(false)
        { }


        // Constructs a vector of the specified size.
        template<typename... Args>
        Vector(bool isFixedSize, Args&&... args)
            : mVector(std::forward<Args>(args)...),
            isFixedSize(isFixedSize),
            isChanged(false)
        { }


        // Checks whether this vector is fixed or resizable.
        bool IsFixedSize() const
        {
            return isFixedSize;
        }


        // Checks whether the contents of the vector have changed since the last call to SetChanged(false).
        bool IsChanged() const
        {
            return isChanged;
        }


        // Sets or clears the IsChanged flag.
        void SetChanged(bool changed)
        {
            isChanged = changed;
        }


        // Expose direct access to the internal STL collection. This lets C++ owners
        // bypass the ExceptionBoundary overhead of the public WinRT API surface.
        typename Traits::InternalVectorType& InternalVector()
        {
            return mVector;
        }


        virtual HRESULT STDMETHODCALLTYPE get_Size(_Out_ unsigned *size)
        {
            return ExceptionBoundary([&]
            {
                CheckInPointer(size);

                *size = Traits::GetSize(mVector);
            });
        };


        virtual HRESULT STDMETHODCALLTYPE GetAt(_In_opt_ unsigned index, _Out_ T_abi *item)
        {
            return ExceptionBoundary([&]
            {
                CheckInPointer(item);
                ZeroMemory(item, sizeof(*item));

                Traits::Unwrap(Traits::GetAt(mVector, index), item);
            });
        }


        virtual HRESULT STDMETHODCALLTYPE IndexOf(_In_opt_ T_abi value, _Out_ unsigned *index, _Out_ boolean *found)
        {
            return ExceptionBoundary([&]
            {
                CheckInPointer(index);
                CheckInPointer(found);

                *index = 0;
                *found = false;

                auto size = Traits::GetSize(mVector);

                for (unsigned i = 0; i < size; i++)
                {
                    if (Traits::Equals(Traits::GetAt(mVector, i), value))
                    {
                        *index = i;
                        *found = true;
                        break;
                    }
                }
            });
        }


        virtual HRESULT STDMETHODCALLTYPE SetAt(_In_ unsigned index, _In_opt_ T_abi item)
        {
            return ExceptionBoundary([&]
            {
                Traits::SetAt(mVector, index, item);
                isChanged = true;
            });
        }


        virtual HRESULT STDMETHODCALLTYPE InsertAt(_In_ unsigned index, _In_opt_ T_abi item)
        {
            return ExceptionBoundary([&]
            {
                if (isFixedSize)
                    ThrowHR(E_NOTIMPL);

                Traits::InsertAt(mVector, index, item);
                isChanged = true;
            });
        }


        virtual HRESULT STDMETHODCALLTYPE RemoveAt(_In_ unsigned index)
        {
            return ExceptionBoundary([&]
            {
                if (isFixedSize)
                    ThrowHR(E_NOTIMPL);

                Traits::RemoveAt(mVector, index);
                isChanged = true;
            });
        }


        virtual HRESULT STDMETHODCALLTYPE Append(_In_opt_ T_abi item)
        {
            return ExceptionBoundary([&]
            {
                if (isFixedSize)
                    ThrowHR(E_NOTIMPL);

                Traits::Append(mVector, item);
                isChanged = true;
            });
        }


        virtual HRESULT STDMETHODCALLTYPE RemoveAtEnd()
        {
            return ExceptionBoundary([&]
            {
                if (isFixedSize)
                    ThrowHR(E_NOTIMPL);

                Traits::RemoveAt(mVector, Traits::GetSize(mVector) - 1);
                isChanged = true;
            });
        }


        virtual HRESULT STDMETHODCALLTYPE Clear()
        {
            return ExceptionBoundary([&]
            {
                if (isFixedSize)
                    ThrowHR(E_NOTIMPL);

                Traits::Clear(mVector);
                isChanged = true;
            });
        }


        virtual HRESULT STDMETHODCALLTYPE ReplaceAll(_In_ unsigned count, _In_reads_(count) T_abi *value)
        {
            return ExceptionBoundary([&]
            {
                CheckInPointer(value);

                if (count == Traits::GetSize(mVector))
                {
                    for (unsigned i = 0; i < count; i++)
                    {
                        Traits::SetAt(mVector, i, value[i]);
                    }
                }
                else
                {
                    if (isFixedSize)
                        ThrowHR(E_NOTIMPL);

                    Traits::Clear(mVector);

                    for (unsigned i = 0; i < count; i++)
                    {
                        Traits::Append(mVector, value[i]);
                    }
                }

                isChanged = true;
            });
        }


        virtual HRESULT STDMETHODCALLTYPE GetView(_Outptr_result_maybenull_ ABI::Windows::Foundation::Collections::IVectorView<T> **view)
        {
            return ExceptionBoundary([&]
            {
                CheckAndClearOutPointer(view);

                auto vectorView = Microsoft::WRL::Make<VectorView<T, Vector>>(this);
                CheckMakeResult(vectorView);

                *view = vectorView.Detach();
            });
        }


        virtual HRESULT STDMETHODCALLTYPE First(_Outptr_result_maybenull_ ABI::Windows::Foundation::Collections::IIterator<T> **first)
        {
            return ExceptionBoundary([&]
            {
                CheckAndClearOutPointer(first);

                auto iterator = Microsoft::WRL::Make<VectorIterator<T, Vector>>(this);
                CheckMakeResult(iterator);

                *first = iterator.Detach();
            });
        }
    };


    // Implements the WinRT IVectorView interface.
    template<typename T, typename TVector>
    class VectorView : public Microsoft::WRL::RuntimeClass<ABI::Windows::Foundation::Collections::IVectorView<T>,
        ABI::Windows::Foundation::Collections::IIterable<T>>,
        private LifespanTracker<VectorView<T, TVector>>
    {
        InspectableClass(IVectorView<T>::z_get_rc_name_impl(), BaseTrust);

        // Fields.
        Microsoft::WRL::ComPtr<TVector> mVector;


    public:
        // Constructor wraps around an existing Vector<T>.
        VectorView(TVector* vector)
            : mVector(vector)
        { }


        virtual HRESULT STDMETHODCALLTYPE get_Size(_Out_ unsigned *size)
        {
            return mVector->get_Size(size);
        };


        virtual HRESULT STDMETHODCALLTYPE GetAt(_In_opt_ unsigned index, _Out_ typename TVector::T_abi *item)
        {
            return mVector->GetAt(index, item);
        }


        virtual HRESULT STDMETHODCALLTYPE IndexOf(_In_opt_ typename TVector::T_abi value, _Out_ unsigned *index, _Out_ boolean *found)
        {
            return mVector->IndexOf(value, index, found);
        }


        virtual HRESULT STDMETHODCALLTYPE First(_Outptr_result_maybenull_ ABI::Windows::Foundation::Collections::IIterator<T> **first)
        {
            return mVector->First(first);
        }
    };


    // Implements the WinRT IIterator interface.
    template<typename T, typename TVector>
    class VectorIterator : public Microsoft::WRL::RuntimeClass<ABI::Windows::Foundation::Collections::IIterator<T>>,
        private LifespanTracker<VectorIterator<T, TVector>>
    {
        InspectableClass(IIterator<T>::z_get_rc_name_impl(), BaseTrust);

        // Fields.
        Microsoft::WRL::ComPtr<TVector> mVector;
        unsigned mPosition;


    public:
        // Constructor wraps around an existing Vector<T>.
        VectorIterator(TVector* vector)
            : mVector(vector),
            mPosition(0)
        { }


        virtual HRESULT STDMETHODCALLTYPE get_Current(_Out_ typename TVector::T_abi *current)
        {
            return mVector->GetAt(mPosition, current);
        }


        virtual HRESULT STDMETHODCALLTYPE get_HasCurrent(_Out_ boolean *hasCurrent)
        {
            return ExceptionBoundary([&]
            {
                CheckInPointer(hasCurrent);

                *hasCurrent = (mPosition < TVector::Traits::GetSize(mVector->InternalVector()));
            });
        }


        virtual HRESULT STDMETHODCALLTYPE MoveNext(_Out_ boolean *hasCurrent)
        {
            return ExceptionBoundary([&]
            {
                CheckInPointer(hasCurrent);

                auto size = TVector::Traits::GetSize(mVector->InternalVector());

                if (mPosition >= size)
                {
                    ThrowHR(E_BOUNDS);
                }

                mPosition++;
                *hasCurrent = (mPosition < size);
            });
        }
    };
}
