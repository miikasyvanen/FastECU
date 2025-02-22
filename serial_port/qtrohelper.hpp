// This is an open source non-commercial project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#ifndef QTROHELPER_HPP
#define QTROHELPER_HPP

#include <QRemoteObjectPendingCallWatcher>

namespace qtrohelper
{
//Convert QVariant to scalar templates
template <typename T>
T qvariant_to_scalar(QVariant v);

template <>
inline long qvariant_to_scalar<long>(QVariant v)
{
    return v.toLongLong();
}

template<>
inline unsigned long qvariant_to_scalar<unsigned long>(QVariant v)
{
    return v.toULongLong();
}

template <>
inline int qvariant_to_scalar<int>(QVariant v)
{
    return v.toInt();
}

template <>
inline unsigned int qvariant_to_scalar<unsigned int>(QVariant v)
{
    return v.toUInt();
}

template <>
inline unsigned char qvariant_to_scalar<unsigned char>(QVariant v)
{
    return v.toInt();
}

template <>
inline bool qvariant_to_scalar<bool>(QVariant v)
{
    return v.toBool();
}

template <>
inline QString qvariant_to_scalar<QString>(QVariant v)
{
    return v.toString();
}

template <>
inline QByteArray qvariant_to_scalar<QByteArray>(QVariant v)
{
    return v.toByteArray();
}

template <>
inline QStringList qvariant_to_scalar<QStringList>(QVariant v)
{
    return v.toStringList();
}

/*
 * Do syncronous call of remote object method
 *
 * Note that QRemoteObjectPendingReply is template class.
 * This template first type argument must be
 * class QRemoteObjectPendingReply<return_type>
 * Second type argument is returning type of remote object method
 * For example source class implementation is:
 *
 * class QtroRemote : public QtroRemoteSimpleSource
 *   {
 *       Q_OBJECT
 *   public:
 *       long someFunc(QString s) override;
 *   };
 *
 *   On replica:
 *
 *  QScopedPointer<QtroRemoteReplica> qtro_remote(
 *       node.acquire<QtroRemoteReplica>("source_name")
 *   );
 *
 *   Then type of 'qtro_remote->someFunc()' will be deduced as
 *   QRemoteObjectPendingReply<long>.
 *   Note that 'qtro_remote->someFunc()' return type is 'long'.
 *   And 'slot_sync(qtro_remote->someFunc("text"))' call deduces to
 *
 *   slot_sync<QRemoteObjectPendingReply<long>, long>(qtro_remote->someFunc("text"))
*/
template <template <typename> typename QRemoteObjectPendingReply, typename RET_TYPE>
RET_TYPE slot_sync(QRemoteObjectPendingReply<RET_TYPE> SLOT)
{
    bool error;
    int retry_count = 0;
    QVariant r;
    do {
        QScopedPointer<QRemoteObjectPendingCallWatcher>
            watcher{new QRemoteObjectPendingCallWatcher(SLOT)};
        QObject::connect(watcher.data(), &QRemoteObjectPendingCallWatcher::finished,
                         watcher.data(), [&](QRemoteObjectPendingCallWatcher* watch)
                         {
                             r = watch->returnValue();
                         }, Qt::DirectConnection);
        watcher->waitForFinished();
        error = watcher->error();
        if (error)
        {
            qDebug() << "Remote call finished with error, retrying...";
            retry_count++;
        }
    } while (error);
    if (retry_count > 0)
        qDebug() << "Call successfuly finished after" << retry_count << "attempts";
    return qvariant_to_scalar<RET_TYPE>(r);
}

} //namespace qtrohelper

#endif // QTROHELPER_HPP
