/* Copyright 2021 Marzocchi Alessandro

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "test.h"
#include "HFSQtLi.h"
#include "NameType.h"

QTEST_MAIN(TestHFSqlite)
/// \cond INTERNAL

#include <QDebug>

using namespace HFSQtLi;


TestHFSqlite::TestHFSqlite()
{
  m_tempFile=QDir(QDir::tempPath()).absoluteFilePath("TestHFSqlite_Temporary.temp_dabatase");
  QFile::remove(m_tempFile);
  qDebug()<<"Selected"<<m_tempFile;
}

struct TestType
{
  TestType(int value): m_value(value)
  {
    m_id=g_objCounter++;
    g_numNewImmediate++;
    if(g_debug)
      qDebug()<<"Created"<<desc().toUtf8().data()<<"="<<value;
  }
  TestType(const TestType &source): m_value(source.m_value)
  {
    m_id=g_objCounter++;
    g_numNewCopy++;
    if(g_debug)
      qDebug()<<"Copy created"<<desc().toUtf8().data()<<"<="<<source.desc().toUtf8().data();
  }
  TestType(TestType &&source): m_value(source.m_value)
  {
    m_id=g_objCounter++;
    g_numNewMove++;
    source.m_value=0;
    if(g_debug)
      qDebug()<<"Move created"<<desc().toUtf8().data()<<"<="<<source.desc().toUtf8().data();
  }
  TestType(): m_value(0)
  {
    m_id=g_objCounter++;
    g_numNewDefault++;
    if(g_debug)
      qDebug()<<"Created"<<desc().toUtf8().data();
  }
  TestType &operator=(int value)
  {
    m_value=value;
    g_numSetImmediate++;
    if(g_debug)
      qDebug()<<"Set"<<desc().toUtf8().data()<<"="<<value;
    return *this;
  }
  TestType &operator=(const TestType &source)
  {
    m_value=source.m_value;
    g_numSetCopy++;
    if(g_debug)
      qDebug()<<"Set"<<desc().toUtf8().data()<<"="<<source.desc().toUtf8().data()<<"("<<m_value<<")";
    return *this;
  }
  TestType &operator=(TestType &&source)
  {
    m_value=source.m_value;
    source.m_value=0;
    g_numSetMove++;
    if(g_debug)
      qDebug()<<"Moved"<<desc().toUtf8().data()<<"="<<source.desc().toUtf8().data()<<"("<<m_value<<")";
    return *this;
  }
  ~TestType()
  {
    if(g_debug)
      qDebug()<<"Destroyed"<<desc().toUtf8().data();
  }
  int value() const
  {
    return m_value;
  }
  QString desc() const
  {
    QString ret;
    int j=m_id;
    do
    {
      ret.insert(0,QChar('A'+(j%26)));
      j/=26;
    } while(j!=0);
    return ret;
  }
  static void reset(bool debug)
  {
    g_debug=debug;
    g_objCounter=0;
  }
  static void reset()
  {
    g_objCounter=0;
    g_numNewImmediate=g_numNewCopy=g_numNewMove=g_numNewDefault=0;
    g_numSetImmediate=g_numSetCopy=g_numSetMove=0;
  }
  static inline int numNewImmediate() { return g_numNewImmediate; }
  static inline int numNewCopy() { return g_numNewCopy; }
  static inline int numNewMove() { return g_numNewMove; }
  static inline int numNewDefault() { return g_numNewDefault; }
  static inline int numSetImmediate() { return g_numSetImmediate; }
  static inline int numSetCopy() { return g_numSetCopy; }
  static inline int numSetMove() { return g_numSetMove; }
  static inline int numOperations() { return g_numNewImmediate+g_numNewCopy+g_numNewMove+g_numNewDefault+g_numSetImmediate+g_numSetCopy+g_numSetMove; }
protected:
  int m_id;
  int m_value;
  static bool g_debug;
  static int g_objCounter;
  static int g_numNewImmediate, g_numNewCopy, g_numNewMove, g_numNewDefault;
  static int g_numSetImmediate, g_numSetCopy, g_numSetMove;
};

bool TestType::g_debug=false;
int TestType::g_objCounter=0;
int TestType::g_numNewImmediate=0, TestType::g_numNewCopy=0, TestType::g_numNewMove=0, TestType::g_numNewDefault=0;
int TestType::g_numSetImmediate=0, TestType::g_numSetCopy=0, TestType::g_numSetMove=0;

void customBindConst(CustomBind &bind, const TestType &type)
{
  bind.bind(type.value()+1);
}

void customFetch(CustomFetch &fetch, TestType &type)
{
  int val;
  if(fetch.fetch(val))
    type=val-1;
}

#ifndef DEVELOPING
void TestHFSqlite::test06Performance()
{
  QScopedPointer<Db> db(Db::open(":memory:", QIODevice::ReadWrite));
  Query qry(db.data());
  QVERIFY(qry.prepare("SELECT $1"));
  TestType val;

  TestType::reset();
  qry.bindAll(TestType(4));
  QCOMPARE(TestType::numNewImmediate(),1);
  QCOMPARE(TestType::numOperations(),1);

  TestType::reset();
  qry.step(val);
  QCOMPARE(TestType::numSetImmediate(),1);
  QCOMPARE(TestType::numOperations(),1);

  TestType::reset();
  qry.executeSingle<1>(TestType(13), val);
  QCOMPARE(TestType::numNewImmediate(),1);
  QCOMPARE(TestType::numSetImmediate(),1);
  QCOMPARE(TestType::numOperations(),2);

  TestType::reset();
  db->executeSingleAll<1>("SELECT $1",TestType(7), val);
  QCOMPARE(TestType::numNewImmediate(),1);
  QCOMPARE(TestType::numSetImmediate(),1);
  QCOMPARE(TestType::numOperations(),2);
}

template <int ...Is> struct NameType::NameOfBaseType<Helper::int_sequence<Is...> >
{
  static std::string name()
  {
    auto l=QList{Is...};
    QStringList ret;
    for(int i=0;i<l.size();i++)
      ret.append(QString::number(l[i]));
    return QString("int_seq(%1)").arg(ret.join(", ")).toStdString();
  }
};

void TestHFSqlite::test05Blob()
{
  QScopedPointer<Db> db(Db::open(":memory:", QIODevice::ReadWrite));
  QByteArray data;
  char temp[50];
  QVERIFY(db->executeSingleAll<1>("SELECT $1", ZeroBlob(5), data));
  QCOMPARE(data, QByteArray(5, 0));

  QByteArray hello("Hello"), world("World!"), foo("Foo");
  QVERIFY(db->executeSingleAll<1>("SELECT $1", hello, data));
  QCOMPARE(data, hello);

  QVERIFY(db->execute("CREATE TABLE test (id INTEGER PRIMARY KEY, blob)"));
  QVERIFY(db->execute("INSERT INTO test(id,blob) VALUES ($1, $2)", 1, hello));
  QVERIFY(db->execute("INSERT INTO test(id,blob) VALUES ($1, $2)", 2, world));
  {
    Blob blob(false, true); // Read only, auto open
    QVERIFY(blob.setDbPointer(db.data()));
    QVERIFY(blob.setTable("test"));
    QVERIFY(blob.setColumn("blob"));
    QVERIFY(!blob.isOpen());
    // Row 1
    QVERIFY(blob.setRowId(1));
    QVERIFY(blob.isOpen());
    QCOMPARE(blob.size(), 5);

    QVERIFY(!blob.write(foo));
    QVERIFY(blob.read(temp, hello.size())); // Size
    QCOMPARE(QByteArray(temp, hello.size()), hello);

    QVERIFY(!blob.read(temp, hello.size()+1)); // Invalid size, offset
    QVERIFY(!blob.read(temp, hello.size(), 1)); // Invalid size, offset

    QVERIFY(blob.read(temp, hello.size()-2, 2)); // Read in mid
    QCOMPARE(QByteArray(temp, hello.size()-2), hello.mid(2));

    QVERIFY(blob.read(temp, hello.size(), 0)); // Size, offset
    QCOMPARE(QByteArray(temp, hello.size()), hello);
    QCOMPARE(blob.read(hello.size()), hello);
    QCOMPARE(blob.read(hello.size()+1), QByteArray());
    QCOMPARE(blob.read(hello.size(),1), QByteArray());
    QCOMPARE(blob.read(hello.size()-2,2), hello.mid(2));
    QCOMPARE(blob.readAll(), hello);

    QVERIFY(blob.setRowId(2));
    QVERIFY(blob.isOpen());
    QCOMPARE(blob.size(), 6);
    QVERIFY(!blob.write(foo));
    QCOMPARE(blob.readAll(), world);

    QVERIFY(!blob.setRowId(3));
    QVERIFY(!blob.isOpen());
    QVERIFY(blob.size()<0);

    QVERIFY(blob.setRowId(1));
    QCOMPARE(blob.readAll(), hello);
  }
  {
    Blob blob(true, true); // Read/write, auto open, not fast
    QVERIFY(blob.setDbPointer(db.data()));
    QVERIFY(blob.setTable("test"));
    QVERIFY(blob.setColumn("blob"));
    QVERIFY(!blob.isOpen());
    // Row 1
    QVERIFY(blob.setRowId(1));
    QVERIFY(blob.isOpen());
    QVERIFY(blob.write(foo));
    QCOMPARE(blob.readAll(), QByteArray("Foolo"));
    QVERIFY(!blob.write("bar", 3, 3));
    QCOMPARE(blob.readAll(), QByteArray("Foolo"));
    QVERIFY(blob.write("ba", 2, 3));
    QCOMPARE(blob.readAll(), QByteArray("Fooba"));
    QVERIFY(blob.write(hello));
    QVERIFY(blob.setRowId(2));
    QVERIFY(blob.write(foo));
    QVERIFY(blob.setRowId(1));
    QCOMPARE(blob.readAll(), hello);
    QVERIFY(!blob.setRowId(5));
    QVERIFY(!blob.isOpen());
    QVERIFY(blob.size()<0);
    QVERIFY(blob.setRowId(2));
    QCOMPARE(blob.readAll(), QByteArray("Foold!"));
    QVERIFY(blob.write(world));
    QCOMPARE(blob.readAll(), world);
  }
  {
    Blob blob(true, true); // Read/write, auto open
    QVERIFY(blob.setDbPointer(db.data()));
    QVERIFY(blob.setTable("test"));
    QVERIFY(blob.setColumn("blob"));
    QVERIFY(!blob.isOpen());
    // Row 1
    QVERIFY(blob.setRowId(1));
    QVERIFY(blob.isOpen());
    QVERIFY(blob.write(foo));
    QCOMPARE(blob.readAll(), QByteArray("Foolo"));
    QVERIFY(blob.write(hello));
    QVERIFY(blob.setRowId(2));
    QCOMPARE(blob.readAll(), world);
    QVERIFY(!blob.setRowId(3));
    QVERIFY(!blob.isOpen());
    QVERIFY(!blob.reopenFast(2));
    QCOMPARE(blob.readAll(), QByteArray());

    QCOMPARE(blob.readAll(), QByteArray());
    QVERIFY(blob.setRowId(1));
    QCOMPARE(blob.readAll(), hello);
    QVERIFY(blob.reopenFast(2));
    QCOMPARE(blob.readAll(), world);
  }
  {
    Blob blob(false, false); // Read only, not auto open
    QVERIFY(blob.setDbPointer(db.data()));
    QVERIFY(blob.setTable("test"));
    QVERIFY(blob.setColumn("blob"));
    QVERIFY(!blob.isOpen());
    // Row 1
    QVERIFY(blob.setRowId(1));
    QVERIFY(!blob.isOpen());
    QVERIFY(blob.open());
    QCOMPARE(blob.readAll(), hello);

    QVERIFY(blob.setRowId(3));
    QVERIFY(!blob.isOpen());
    QVERIFY(!blob.open());
    QCOMPARE(blob.readAll(), QByteArray());

    QVERIFY(blob.setRowId(2));
    QVERIFY(!blob.isOpen());
    QVERIFY(blob.open());
    QCOMPARE(blob.readAll(), world);
    QVERIFY(blob.reopenFast(1));
    QCOMPARE(blob.readAll(), hello);
  }
}

void TestHFSqlite::test04Call()
{
  QScopedPointer<Db> db(Db::open(":memory:", QIODevice::ReadWrite));
  Query qry(db.data());
  int x=0; double y=0; QString w;
  QVERIFY(qry.prepare("SELECT $1, $2, $3, $4"));
  // Read x,w via lambda
  QVERIFY(qry.executeSingle<4>(1,2.3,3,"Foo", Call<int, UnusedN<2>, QString>([&x, &w](int xp, const QString &wp){x=xp; w=wp; return true;})));
  QCOMPARE(x,1);
  QCOMPARE(w,"Foo");
  // Following should fail: callbacks returns false
  QVERIFY(!qry.executeSingle<4>(1,2.3,3,"Foo", Call<int, UnusedN<2>, QString>([&x, &w](int xp, const QString &wp){x=xp; w=wp; return false;})));
  // Following should fail: read 3 parameters instead of 4
  QVERIFY(!qry.executeSingle<4>(1,2.3,3,"Foo", Call<int, Unused, QString>([&x, &w](int xp, const QString &wp){x=xp; w=wp; return true;})));
  // Read x,y,w via lambda
  QVERIFY(qry.executeSingle<4>(2,3.3,4,"Fee", Call<int, std::tuple<double,Unused,QString>>([&x, &y,&w](int xp, std::tuple<double &,QString &> tup){x=xp; y=std::get<0>(tup); w=std::get<1>(tup); return true;})));
  QCOMPARE(x,2);
  QCOMPARE(y,3.3);
  QCOMPARE(w,"Fee");
}

void TestHFSqlite::init()
{
  QVERIFY(!m_tempFile.isEmpty());
  QFile::remove(m_tempFile);
}

void TestHFSqlite::test00BasicQuery()
{
  QScopedPointer<Db> db(Db::open(":memory:", QIODevice::ReadWrite));
  QVERIFY(db);
  Query qry(db.data());
  int i=0,j=0,k=0;
  QVERIFY(qry.prepare("VALUES (4,5)"));
  QVERIFY(qry.stepNoFetch());
  QVERIFY(!qry.stepNoFetch());
  QCOMPARE(qry.error(), SQLiteCode::DONE);

  QVERIFY(qry.prepare("VALUES (4,5)"));
  QVERIFY(qry.stepNoFetch());
  QVERIFY(qry.reset());
  QVERIFY(qry.stepNoFetch());
  QVERIFY(!qry.stepNoFetch());
  QCOMPARE(qry.error(), SQLiteCode::DONE);

  QVERIFY(qry.prepare("VALUES (4,5),(6,7)"));
  QVERIFY(qry.stepNoFetch());
  QVERIFY(qry.stepNoFetch());
  QVERIFY(!qry.stepNoFetch());
  QCOMPARE(qry.error(), SQLiteCode::DONE);

  QVERIFY(qry.prepare("VALUES (4,5)"));
  QVERIFY(qry.stepNoFetch());
  QCOMPARE(qry.column(0,i),2);
  QCOMPARE(i,4);
  i=0;
  QCOMPARE(qry.column(0,i,j),3);
  QCOMPARE(i,4);
  QCOMPARE(j,5);

  QCOMPARE(qry.columnAll(i),0);
  QCOMPARE(qry.columnAll(i,j,k),0);
  i=0; j=0;
  QCOMPARE(qry.columnAll(i,j),3);
  QCOMPARE(i,4);
  QCOMPARE(j,5);

  QVERIFY(qry.prepare("VALUES ($1,$2)"));
  QCOMPARE(qry.bind(1, 15),2);
  QVERIFY(qry.stepNoFetch());
  QVERIFY(qry.column(0, i));
  QCOMPARE(i,15);
  QVERIFY(qry.reset());
  QCOMPARE(qry.bindAll(7), 0);
  QCOMPARE(qry.bindAll(7, 8, 9), 0);
  QCOMPARE(qry.bindAll(15, 16), 3);
  QVERIFY(qry.stepNoFetch());
  QVERIFY(qry.column(0, i, j));
  QCOMPARE(i,15);
  QCOMPARE(j,16);

  QVERIFY(!qry.prepare("SELECT * FROM myTable"));
  QVERIFY(!qry.isPrepared());
  QVERIFY(!qry.executeCommand());
  QVERIFY(qry.prepare("CREATE TABLE myTable(a TEXT, b TEXT, c TEXT)"));
  QVERIFY(qry.isPrepared());
  QVERIFY(qry.executeCommand());
  QVERIFY(qry.prepare("SELECT * FROM myTable"));
  QVERIFY(qry.isPrepared());
  QVERIFY(qry.executeCommand());
}

void TestHFSqlite::test01BindFetch()
{


  QScopedPointer<Db> db(Db::open(":memory:", QIODevice::ReadWrite));

  // column one-by-one
  {
    int value1=0;
    QString value2;
    Query qry(db.data());
    QVERIFY(qry.prepare("SELECT 4, 'Foo'"));
    QVERIFY(qry.stepNoFetch());
    QCOMPARE(qry.column(0, value1),2);
    QCOMPARE(qry.column(1, value2),2);
    QCOMPARE(value1,4);
    QCOMPARE(value2,"Foo");
  }
  // column many
  {
    int value1=0;
    QString value2;
    Query qry(db.data());
    QVERIFY(qry.prepare("SELECT 4, 'Foo'"));
    QVERIFY(qry.stepNoFetch());
    QCOMPARE(qry.column(0, value1, value2),3);
    QCOMPARE(value1,4);
    QCOMPARE(value2,"Foo");
  }
  // columnAll
  {
    int value1=0;
    QString value2, value3;
    Query qry(db.data());
    QVERIFY(qry.prepare("SELECT 4, 'Foo'"));
    QVERIFY(qry.stepNoFetch());
    QCOMPARE(qry.columnAll(value1, value2), 3);
    QCOMPARE(value1, 4);
    QCOMPARE(value2, "Foo");
    QVERIFY(!qry.columnAll(value1));
    QVERIFY(!qry.columnAll(value1, value2, value3));
  }
  // bind one-by-one
  {
    int value1=0;
    QString value2;
    Query qry(db.data());
    QVERIFY(qry.prepare("SELECT $1, $2"));
    QCOMPARE(qry.bind(1, 4), 2);
    QCOMPARE(qry.bind(2, "Foo"), 2);
    QVERIFY(qry.stepNoFetch());
    QCOMPARE(qry.columnAll(value1, value2), 3);
    QCOMPARE(value1,4);
    QCOMPARE(value2,"Foo");
  }
  // bind multi
  {
    int value1=0;
    QString value2;
    Query qry(db.data());
    QVERIFY(qry.prepare("SELECT $1, $2"));
    QCOMPARE(qry.bind(1, 4, "Foo"),3);
    QVERIFY(qry.stepNoFetch());
    QCOMPARE(qry.columnAll(value1, value2),3);
    QCOMPARE(value1,4);
    QCOMPARE(value2,"Foo");
  }
  // bind all
  {
    int value1=0;
    QString value2;
    Query qry(db.data());
    QVERIFY(qry.prepare("SELECT $1, $2"));
    QVERIFY(!qry.bindAll(4));
    QVERIFY(!qry.bindAll(4, 5, "Foo"));
    QCOMPARE(qry.bindAll(4, "Foo"),3);
    QVERIFY(qry.stepNoFetch());
    QCOMPARE(qry.columnAll(value1, value2),3);
    QCOMPARE(value1,4);
    QCOMPARE(value2,"Foo");
  }
  // excecuteSingle
  {
    Query qry(db.data());
    QVERIFY(qry.prepare("VALUES (3,4)"));
    int i=0,j=0;
    QCOMPARE(qry.executeSinglePartial(0,i),2);
    QCOMPARE(i,3);
    QCOMPARE(qry.executeSinglePartial(1,i), 2);
    QCOMPARE(i,4);
    QVERIFY(!qry.executeSingle(i));
    QCOMPARE(qry.executeSingle(i,j),3);
    QCOMPARE(i,3);
    QCOMPARE(j,4);
    QVERIFY(qry.prepare("VALUES (3,4),(4,5),(6,7)"));
    QVERIFY(!qry.executeSinglePartial(0,i));
    QVERIFY(!qry.executeSingle(i,j));
    QVERIFY(qry.prepare("VALUES ($1,$2)"));
    QCOMPARE(qry.bind(2, 4),2);
    QCOMPARE(qry.executeSinglePartial<1>(1, 0, 7, i),2);
    QCOMPARE(i,7);
    QCOMPARE(qry.executeSinglePartial<1>(1, 1, 9, i),2);
    QCOMPARE(i,4);
    i=4;
    QVERIFY(!qry.executeSingle<2>(12, 13, i));
    QVERIFY(!qry.executeSingle<1>(14, i, j));
    QCOMPARE(qry.executeSingle<2>(15, 16, i, j),3);
    QCOMPARE(i,15);
    QCOMPARE(j,16);
  }
  // excecuteSingle directly on database
  {
    int i=0,j=0;
    QVERIFY(!db->executeSingleAll<0>("SELECT 4,5 LIMIT 0", i,j)); // Multiple values
    QVERIFY(!db->executeSingleAll<0>("VALUES (3,4),(4,5),(6,7)", i,j)); // Multiple values
    QVERIFY(!db->executeSingleAll<2>("VALUES ($1,$2)", 12, 13, i)); // Unread value
    QVERIFY(!db->executeSingleAll<1>("VALUES ($1,$2)", 14, i, j)); // Unbound parameter
    QCOMPARE(db->executeSingleAll<2>("VALUES ($1,$2)", 15, 16, i, j),3); // Successfull
    QCOMPARE(i,15);
    QCOMPARE(j,16);
  }

}
void TestHFSqlite::test02ColumnTypes()
{
  QScopedPointer<Db> db(Db::open(":memory:", QIODevice::ReadWrite));
  Query qry(db.data());
  QVERIFY(qry.prepare("SELECT $1"));
  int dInt=-12; QString dQString; double dDouble=qQNaN(); Value dVal; TestType dTest;
  std::optional<int> dOInt;
  QVERIFY(qry.executeSingle<1>(Null(), dVal));
  QVERIFY(dVal.isNull());
  QVERIFY(qry.executeSingle<1>(15, dVal));
  QVERIFY(dVal.isInt());
  QCOMPARE(dVal.toInt64(), 15);
  QVERIFY(qry.executeSingle<1>("Foo", dVal));
  QVERIFY(dVal.isText());
  QCOMPARE(dVal.toString(), "Foo");
  QVERIFY(qry.executeSingle<1>(4.2, dVal));
  QVERIFY(dVal.isFloat());
  QCOMPARE(dVal.toDouble(), 4.2);
  QVERIFY(qry.executeSingle<1>(QByteArray(), dVal));
  QVERIFY(dVal.isBlob());
  // TODO dVal.toByteArray()
  QVERIFY(qry.executeSingle<1>(nullptr, dVal));
  QVERIFY(dVal.isNull());

  QVERIFY(qry.executeSingle<1>(4, dInt));

  QCOMPARE(dInt, 4);
  QVERIFY(qry.executeSingle<1>("Foo", dQString));

  QCOMPARE(dQString,"Foo");
  QVERIFY(qry.executeSingle<1>("5", dInt));
  QCOMPARE(dInt, 5);

  QVERIFY(qry.executeSingle<1>(3.3, dDouble));
  QCOMPARE(dDouble, 3.3);

  QVERIFY(qry.executeSingle<1>(6.3, Unused()));

  QVERIFY(qry.executeSingle<1>(12, dOInt));
  QVERIFY(dOInt.has_value());
  QCOMPARE(dOInt, 12);

  QVERIFY(qry.executeSingle<1>(nullptr, dOInt));
  QVERIFY(!dOInt.has_value());

  QVERIFY(qry.executeSingle<1>(TestType(19), dInt)); // Bind of custom type
  QCOMPARE(dInt, 20);


  QVERIFY(qry.executeSingle<1>(12, dTest)); // Fetch of custom type
  QCOMPARE(dTest.value(), 11);

  QVERIFY(qry.prepare("SELECT $1, $2, $3"));
  {
    std::tuple<int,QString,double> data;
    QVERIFY(qry.executeSingle<3>(4,"Foo",3.2,data));
    QCOMPARE(std::get<0>(data), 4);
    QCOMPARE(std::get<1>(data), "Foo");
    QCOMPARE(std::get<2>(data), 3.2);
  }

}

template <typename ...Args> void testf2(Args &&...args)
{
  qDebug()<<"testf2"<<NameType::packName<decltype(args)...>().data();

}
template <typename ...Args> void testf(Args &&...args)
{
  qDebug()<<"testf"<<NameType::packName<decltype(args)...>().data();
  testf2(std::forward<Args>(args)...);
}

void TestHFSqlite::test03StrictColumnTypes()
{
//  int i=4;
//  QString x;
//  const int j=5;
//  testf(4, j, i);
  Query qry;
//  int j;
  qry.readColumn(false, 0, Unused());
//  QScopedPointer<Db> db(Db::open(":memory:", QIODevice::ReadWrite));
//  Query qry(db.data());
//  QVERIFY(qry.prepare("SELECT $1"));
//  int dInt=-12; QString dQString; double dDouble=qQNaN(); Value dVal;
//  std::optional<int> dOInt;
//  QVERIFY(!qry.executeSingleStrict<1>(Null(), dVal));

//  QVERIFY(!qry.executeSingle<1>("5", dInt));

//  QVERIFY(qry.executeSingleStrict<1>(4, dInt));
//  QCOMPARE(dInt, 4);

  //    QScopedPointer<Db> db(Db::open(":memory:", QIODevice::ReadWrite));
//    Query qry(db.data());
//    QVERIFY(qry.prepare("SELECT $1"));
//    int dInt=-12; QString dQString; double dDouble=qQNaN(); Value dVal;
//    std::optional<int> dOInt;
//    QVERIFY(qry.executeSingleAll<1>(Null(), dVal));
//    QVERIFY(dVal.isNull());
//    QVERIFY(qry.executeSingleAll<1>(15, dVal));
//    QVERIFY(dVal.isInt());
//    QCOMPARE(dVal.toInt64(), 15);
//    QVERIFY(qry.executeSingleAll<1>("Foo", dVal));
//    QVERIFY(dVal.isText());
//    QCOMPARE(dVal.toString(), "Foo");
//    QVERIFY(qry.executeSingleAll<1>(4.2, dVal));
//    QVERIFY(dVal.isFloat());
//    QCOMPARE(dVal.toDouble(), 4.2);
//    QVERIFY(qry.executeSingleAll<1>(QByteArray(), dVal));
//    QVERIFY(dVal.isBlob());
//    // TODO dVal.toByteArray()
//    QVERIFY(qry.executeSingleAll<1>(nullptr, dVal));
//    QVERIFY(dVal.isNull());

//    QVERIFY(qry.executeSingleAll<1>(4, dInt));

//    QCOMPARE(dInt, 4);
//    QVERIFY(qry.executeSingleAll<1>("Foo", dQString));

//    QCOMPARE(dQString,"Foo");
//    QVERIFY(qry.executeSingleAll<1>("5", dInt));
//    QCOMPARE(dInt, 5);

//    QVERIFY(qry.executeSingleAll<1>(3.3, dDouble));
//    QCOMPARE(dDouble, 3.3);

//    QVERIFY(qry.executeSingleAll<1>(6.3, Unused()));

//    QVERIFY(qry.executeSingleAll<1>(12, dOInt));
//    QVERIFY(dOInt.has_value());
//    QCOMPARE(dOInt, 12);

//    QVERIFY(qry.executeSingleAll<1>(nullptr, dOInt));
//    QVERIFY(!dOInt.has_value());
}

template <int I> static void testValidSelect()
{
  Query qry;
  int a;
  qry.column(0, a);
}

// CHECK_METHOD(method_name, const_name) will create a templated constant const_name such that:
// const_name<ret-type, class-name, args-type...> that is true if:
//   class_name::method_name(args-type0, args-type1, ..., args-typen) exists and return type ret-type
//
// e.g.
// CHECK_METHOD(find, has_find);
// std::cout<<has_find<size_t, std::string, char, size_t>; // Prints "true"
// std::cout<<has_find<size_t, std::string, char, std::string>; // Prints "false" std::string has no find(char, std::string)
// std::cout<<has_find<size_t, std::vector<int>, char, size_t>; // Prints "false" std:vector<int> has no find
#define CHECK_METHOD(method_name, function_name) \
  template< typename ...> struct function_name##HelperTemplateClass: std::false_type { }; \
  template<typename RET, typename CLASS, typename ...PARAMS > \
    struct function_name##HelperTemplateClass \
      < RET, CLASS, \
        typename std::enable_if< std::is_same< \
          decltype( std::declval<CLASS>().method_name( std::declval<PARAMS>()... ) ) , RET > ::value >::type \
        , PARAMS...> : std::true_type {}; \
  template<typename RET,  typename CLASS, typename ...PARAMS > constexpr bool function_name=function_name##HelperTemplateClass<RET, CLASS, void, PARAMS...>::value;

CHECK_METHOD(readColumn, has_column2);
template <typename ...PARAMS> constexpr bool qry_has_column=has_column2<bool, Query, PARAMS...>;
CHECK_METHOD(column, has_column3);


void TestHFSqlite::cleanup()
{
  QFile::remove(m_tempFile);
}

#endif



/// \endcond INTERNAL
