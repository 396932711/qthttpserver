/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtHttpServer module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QHTTPSERVERROUTERVIEWTRAITS_H
#define QHTTPSERVERROUTERVIEWTRAITS_H

#include <QtCore/qglobal.h>
#include <QtCore/qnamespace.h>

#include <functional>
#include <tuple>
#include <type_traits>

QT_BEGIN_NAMESPACE

class QHttpServerRequest;
class QHttpServerResponder;

template <typename ViewHandler>
struct QHttpServerRouterViewTraits : QHttpServerRouterViewTraits<decltype(&ViewHandler::operator())> {};

template <typename ClassType, typename Ret, typename ... Args>
struct QHttpServerRouterViewTraits<Ret(ClassType::*)(Args...) const>
{
    static constexpr const auto ArgumentCount = sizeof ... (Args);

    template <int I>
    struct Arg {
        using OriginalType = typename std::tuple_element<I, std::tuple<Args...>>::type;
        using Type = typename QtPrivate::RemoveConstRef<OriginalType>::Type;

        static constexpr int metaTypeId() noexcept {
            return qMetaTypeId<
                typename std::conditional<
                    !QMetaTypeId2<Type>::Defined,
                    void,
                    Type>::type>();
        }
    };

private:
    // Tools used to compute ArgumentCapturableCount
    template<typename T>
    static constexpr typename std::enable_if<QMetaTypeId2<T>::Defined, int>::type
            capturable()
    { return 1; }

    template<typename T>
    static constexpr typename std::enable_if<!QMetaTypeId2<T>::Defined, int>::type
            capturable()
    { return 0; }

    static constexpr std::size_t sum() noexcept { return 0; }

    template<typename ... N>
    static constexpr std::size_t sum(const std::size_t it, N ... n) noexcept
    { return it + sum(n...); }

public:
    static constexpr const auto ArgumentCapturableCount =
        sum(capturable<typename QtPrivate::RemoveConstRef<Args>::Type>()...);
    static constexpr const auto ArgumentPlaceholdersCount = ArgumentCount - ArgumentCapturableCount;

private:
    // Tools used to get BindableType
    template<typename Return, typename ... ArgsX>
    struct BindTypeHelper {
        using Type = std::function<Return(ArgsX...)>;
    };

    template<int ... Idx>
    static constexpr auto bindTypeHelper(QtPrivate::IndexesList<Idx...>) ->
        BindTypeHelper<Ret, typename Arg<ArgumentCapturableCount + Idx>::OriginalType...>;

public:
    // C++11 does not allow use of "auto" as a function return type.
    // BindableType is an emulation of "auto" for QHttpServerRouter::bindCapture.
    using BindableType = typename decltype(
            bindTypeHelper(typename QtPrivate::Indexes<ArgumentPlaceholdersCount>::Value{}))::Type;
};

template <typename ClassType, typename Ret>
struct QHttpServerRouterViewTraits<Ret(ClassType::*)() const>
{
    static constexpr const int ArgumentCount = 0;

    template <int I>
    struct Arg {
        using Type = void;
    };

    static constexpr const auto ArgumentCapturableCount = 0;
    static constexpr const auto ArgumentPlaceholdersCount = 0;

    using BindableType = decltype(std::function<Ret()>{});
};

QT_END_NAMESPACE

#endif  // QHTTPSERVERROUTERVIEWTRAITS_H
