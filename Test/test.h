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

#pragma once
#include <QtTest/QtTest>
/// \cond INTERNAL

class TestHFSqlite: public QObject
{
  Q_OBJECT
public:
  TestHFSqlite();
private slots:
#ifndef DEVELOPING
private slots:
  void init();
  void cleanup();
  void test00BasicQuery();
  void test01BindFetch();
  void test02ColumnTypes();
  void test03StrictColumnTypes();
  void test04Call();
  void test05Blob();
  void test06Performance();
#endif
private:
  QString m_tempFile;
};

/// \endcond INTERNAL
