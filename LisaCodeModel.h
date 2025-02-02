#ifndef LISACODEMODEL_H
#define LISACODEMODEL_H

/*
* Copyright 2023 Rochus Keller <mailto:me@rochus-keller.ch>
*
* This file is part of the Lisa Pascal Navigator application.
*
* The following is the license that applies to this copy of the
* application. For a license to use the library under conditions
* other than those described here, please email to me@rochus-keller.ch.
*
* GNU General Public License Usage
* This file may be used under the terms of the GNU General Public
* License (GPL) versions 2.0 or 3.0 as published by the Free Software
* Foundation and appearing in the file LICENSE.GPL included in
* the packaging of this file. Please review the following information
* to ensure GNU General Public Licensing requirements will be met:
* http://www.fsf.org/licensing/licenses/info/GPLv2.html and
* http://www.gnu.org/copyleft/gpl.html.
*/

#include <QAbstractItemModel>
#include <QHash>
#include <FileSystem.h>
#include "LisaRowCol.h"

namespace Lisa
{
class Scope;
class CodeFile;
class Symbol;

class Thing
{
public:
    enum Type { Undefined,
       /* Declaration: */ Const, Type, Var, Func, Proc, Param, Label, Field, TypeAlias,
       /* Scope: */ Interface, Implementation, Body,
       /* CodeFile: */ File,
       /* IncludeFile: */ Include,
       /* CodeFolder: */ Folder
    };
    quint8 d_type;
    bool d_external; // Fund & Proc decls

    virtual RowCol getLoc() const { return RowCol(); }
    virtual QString getFilePath() const { return QString(); }
    virtual quint16 getLen() const { return 0; }
    virtual QString getName() const;
    bool isDeclaration() const { return d_type >= Const && d_type <= TypeAlias; }
    const char* typeName() const;
    Thing():d_type(Undefined),d_external(false){}
    virtual ~Thing();
};

class Declaration : public Thing
{
public:
    Declaration* d_impl; // points to the twin in the implementation if this is in an interface
    Declaration* d_intf; // points to the twin in the interface if this is in an implementation
    Scope* d_body; // owns
    QByteArray d_name;
    RowCol d_loc;
    Scope* d_owner;
    QHash<CodeFile*,QList<Symbol*> > d_refs;

    RowCol getLoc() const { return d_loc; }
    QString getFilePath() const;
    quint16 getLen() const { return d_name.size(); }
    QString getName() const;
    CodeFile* getCodeFile() const;
    Declaration():d_impl(0),d_intf(0),d_body(0),d_owner(0){}
    ~Declaration();
};

class Scope : public Thing
{
public:
    QList<Declaration*> d_order; // owns
    Thing* d_owner; // either declaration or codefile
    Scope* d_outer;
    mutable QHash<QByteArray,Declaration*> d_cache;

    CodeFile* getCodeFile() const;
    Declaration* findDecl(const QByteArray& name , bool withImports = true) const;
    Scope():d_owner(0),d_outer(0){}
    ~Scope();
};

class Symbol
{
public:
    Thing* d_decl;
    RowCol d_loc;
    Symbol():d_decl(0){}
};

class IncludeFile : public Thing
{
public:
    const FileSystem::File* d_file;
    CodeFile* d_includer;
    RowCol d_loc;
    quint16 d_len;

    RowCol getLoc() const { return d_loc; }
    QString getFilePath() const;
    quint16 getLen() const { return d_len; }
    QString getName() const;
    IncludeFile():d_file(0),d_includer(0),d_len(0){ d_type = Include; }
};

class CodeFile : public Thing
{
public:
    Scope* d_intf; // owns, 0 for Program
    Scope* d_impl; // owns
    QList<Symbol*> d_syms; // owns, all things we can click on in a code file ordered by row/col
    const FileSystem::File* d_file;
    QList<IncludeFile*> d_includes; // owns
    QList<CodeFile*> d_import;

    QString getName() const;
    QByteArrayList findUses() const;
    CodeFile():d_intf(0),d_impl(0),d_file(0) { d_type = File; }
    ~CodeFile();
};

class CodeFolder : public Thing
{
public:
    FileSystem::Dir* d_dir;
    QList<CodeFolder*> d_subs; // owns
    QList<CodeFile*> d_files; // owns

    QString getName() const;
    void clear();
    CodeFolder():d_dir(0){ d_type = Folder; }
    ~CodeFolder() { clear(); }
};

class CodeModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit CodeModel(QObject *parent = 0);

    bool load( const QString& rootDir );
    const Thing* getThing(const QModelIndex& index) const;
    Symbol* findSymbolBySourcePos(const QString& path, int line, int col) const;
    FileSystem* getFs() const { return d_fs; }
    quint32 getSloc() const { return d_sloc; }
    CodeFile* getCodeFile(const QString& path) const;

    // overrides
    int columnCount ( const QModelIndex & parent = QModelIndex() ) const { return 1; }
    QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    QModelIndex index ( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
    QModelIndex parent ( const QModelIndex & index ) const;
    int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
    Qt::ItemFlags flags ( const QModelIndex & index ) const;

protected:
    void parseAndResolve(CodeFile*);

private:
    struct Slot
    {
        Thing* d_thing;
        QList<Slot*> d_children;
        Slot* d_parent;
        Slot(Slot* p = 0, Thing* t = 0):d_parent(p),d_thing(t){ if( p ) p->d_children.append(this); }
        ~Slot() { foreach( Slot* s, d_children ) delete s; }
    };
    static bool lessThan( const Slot* lhs, const Slot* rhs);
    void fillFolders(Slot* root, const FileSystem::Dir* super, CodeFolder* top, QList<Slot*>& fileSlots);
    Slot d_root;
    FileSystem* d_fs;
    CodeFolder d_top;
    QHash<const FileSystem::File*,CodeFile*> d_map1;
    QHash<QString,CodeFile*> d_map2; // real path -> file
    quint32 d_sloc; // number of lines of code without empty or comment lines
};
}

#endif // LISACODEMODEL_H
