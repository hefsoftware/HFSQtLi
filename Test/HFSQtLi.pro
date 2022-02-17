QT       += core testlib

# Needed for blob select queries
DEFINES += SQLITE_ENABLE_COLUMN_METADATA


SOURCES += \
    blob.cpp \
    database.cpp \
    query.cpp \
    sqlite3.c \
    test.cpp \
    util.cpp

HEADERS += \
    Doxygen.h \
    HFSQtLi.h \
    NameType.h \
    blob.h \
    database.h \
    database_template.h \
    license.h \
    query.h \
    query_template.h \
    sqlite3.h \
    templatehelper.h \
    test.h \
    util.h \
    util_template.h
