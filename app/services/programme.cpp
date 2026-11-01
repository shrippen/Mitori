#include "programme.h"

Programme::Programme(QObject *parent)
    : QObject(parent)
{
    m_catalog.load(QString::fromLatin1(Catalog::kResource));
}
