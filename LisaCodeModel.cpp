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

#include "LisaCodeModel.h"
#include "PpLexer.h"
#include "LisaParser.h"
#include <QFile>
#include <QPixmap>
#include <QtDebug>
#include <QCoreApplication>
using namespace Lisa;

class CodeModelVisitor
{
    CodeModel* d_mdl;
    CodeFile* d_cf;
public:
    CodeModelVisitor(CodeModel* m):d_mdl(m) {}

    void visit( CodeFile* cf, SynTree* top )
    {      
        d_cf = cf;
        if( top->d_children.isEmpty() )
            return;
        switch(top->d_children.first()->d_tok.d_type)
        {
        case SynTree::R_program_:
            program(cf,top->d_children.first());
            break;
        case SynTree::R_regular_unit:
            regular_unit(cf,top->d_children.first());
            break;
        case SynTree::R_non_regular_unit:
            qWarning() << "SynTree::R_non_regular_unit should no longer happen since we have include";
            break;
        }
    }
    void program( CodeFile* cf, SynTree* st )
    {
        Scope* s = new Scope();
        s->d_owner = cf;
        s->d_type = Thing::Body;
        cf->d_impl = s;
        for( int i = 0; i < st->d_children.size(); i++ )
        {
            switch(st->d_children[i]->d_tok.d_type)
            {
            case SynTree::R_block:
                block(cf->d_impl,st->d_children[i]);
                break;
            case SynTree::R_statement_part:
                statement_part(cf->d_impl,st->d_children[i]);
                break;
            }
        }
    }
    void regular_unit( CodeFile* cf, SynTree* st )
    {
        foreach( SynTree* s, st->d_children )
        {
            if( s->d_tok.d_type == SynTree::R_interface_part)
                interface_part(cf,s);
            if( s->d_tok.d_type == SynTree::R_implementation_part)
                implementation_part(cf,s);
        }
    }
    void interface_part( CodeFile* cf, SynTree* st )
    {
        Scope* newScope = new Scope();
        newScope->d_owner = cf;
        newScope->d_type = Thing::Interface;
        cf->d_intf = newScope;
        foreach( SynTree* s, st->d_children )
        {
            if( s->d_tok.d_type == SynTree::R_constant_declaration_part)
                constant_declaration_part(newScope,s);
            if( s->d_tok.d_type == SynTree::R_type_declaration_part)
                type_declaration_part(newScope,s);
            if( s->d_tok.d_type == SynTree::R_variable_declaration_part)
                variable_declaration_part(newScope,s);
            if( s->d_tok.d_type == SynTree::R_procedure_and_function_declaration_part)
                procedure_and_function_interface_part(newScope,s);
        }
    }
    void procedure_and_function_interface_part(Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
        {
            if( s->d_tok.d_type == SynTree::R_procedure_heading)
                procedure_heading(scope,s);
            if( s->d_tok.d_type == SynTree::R_function_heading)
                function_heading(scope,s);
        }
    }
    void implementation_part( CodeFile* cf, SynTree* st )
    {
        Scope* newScope = new Scope();
        newScope->d_owner = cf;
        newScope->d_type = Thing::Implementation;
        cf->d_impl = newScope;
        foreach( SynTree* s, st->d_children )
        {
            if( s->d_tok.d_type == SynTree::R_constant_declaration_part)
                constant_declaration_part(newScope,s);
            if( s->d_tok.d_type == SynTree::R_type_declaration_part)
                type_declaration_part(newScope,s);
            if( s->d_tok.d_type == SynTree::R_variable_declaration_part)
                variable_declaration_part(newScope,s);
            if( s->d_tok.d_type == SynTree::R_subroutine_part)
                subroutine_part(newScope,s);
        }
    }
    void subroutine_part(Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
        {
            if( s->d_tok.d_type == SynTree::R_procedure_declaration)
                procedure_declaration(scope,s);
            if( s->d_tok.d_type == SynTree::R_function_declaration)
                function_declaration(scope,s);
            if( s->d_tok.d_type == SynTree::R_method_block)
                method_block(scope,s);
        }
    }
    void method_block(Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
        {
            if( s->d_tok.d_type == Tok_identifier)
                addSym(scope,s->d_tok);
            if( s->d_tok.d_type == SynTree::R_procedure_and_function_declaration_part)
                procedure_and_function_declaration_part(scope,s);
            if( s->d_tok.d_type == SynTree::R_statement_part)
                statement_part(scope,s);
        }
    }
    void block( Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
        {
            if( s->d_tok.d_type == SynTree::R_label_declaration_part)
                label_declaration_part(scope,s);
            if( s->d_tok.d_type == SynTree::R_constant_declaration_part)
                constant_declaration_part(scope,s);
            if( s->d_tok.d_type == SynTree::R_type_declaration_part)
                type_declaration_part(scope,s);
            if( s->d_tok.d_type == SynTree::R_variable_declaration_part)
                variable_declaration_part(scope,s);
            if( s->d_tok.d_type == SynTree::R_procedure_and_function_declaration_part)
                procedure_and_function_declaration_part(scope,s);
        }
    }
    void label_declaration_part( Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
            if( s->d_tok.d_type == SynTree::R_label_)
                label_(scope,s);
    }
    void label_( Scope* scope, SynTree* st)
    {
        // TODO: what to do with the digit_sequence?
    }
    void constant_declaration_part( Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
            if( s->d_tok.d_type == SynTree::R_constant_declaration)
                constant_declaration(scope,s);
    }
    void constant_declaration( Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
        {
            if( s->d_tok.d_type == Tok_identifier)
                addDecl(scope, s->d_tok, Thing::Const);
            if( s->d_tok.d_type == SynTree::R_expression)
                expression(scope,s);
        }
    }
    void type_declaration_part( Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
            if( s->d_tok.d_type == SynTree::R_type_declaration)
                type_declaration(scope,s);
    }
    Declaration* addDecl(Scope* scope, const Token& t, int type )
    {
        Declaration* d = new Declaration();
        d->d_type = type;
        d->d_name = t.d_val;
        d->d_loc = t.toLoc();
        d->d_owner = scope;
        scope->d_order.append(d);
        return d;
    }

    void type_declaration( Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
        {
            if( s->d_tok.d_type == Tok_identifier)
                addDecl(scope, s->d_tok, Thing::Type);
            if( s->d_tok.d_type == SynTree::R_type_)
                type_(scope,s);
        }
    }
    void type_( Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
        {
            if( s->d_tok.d_type == SynTree::R_simple_type)
                simple_type(scope,s);
            if( s->d_tok.d_type == SynTree::R_string_type)
                string_type(scope,s);
            if( s->d_tok.d_type == SynTree::R_structured_type)
                structured_type(scope,s);
            if( s->d_tok.d_type == SynTree::R_pointer_type)
                pointer_type(scope,s);
        }
    }
    void simple_type(Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
        {
            if( s->d_tok.d_type == Tok_identifier)
                addSym(scope,s->d_tok);
            if( s->d_tok.d_type == SynTree::R_subrange_type)
                subrange_type(scope,s);
            if( s->d_tok.d_type == SynTree::R_enumerated_type)
                enumerated_type(scope,s);
        }
    }
    void subrange_type(Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
            if( s->d_tok.d_type == SynTree::R_constant)
                constant(scope,s);
    }
    void enumerated_type(Scope* scope, SynTree* st)
    {
        QList<Token> ids = identifier_list(st);
        foreach( const Token& t, ids )
            addDecl(scope,t,Thing::Const);
    }

    void string_type(Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
            if( s->d_tok.d_type == SynTree::R_size_attribute)
                size_attribute(scope,s);
    }
    void size_attribute(Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
            if( s->d_tok.d_type == Tok_identifier)
                addSym(scope,s->d_tok);
    }
    void structured_type(Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
        {
            if( s->d_tok.d_type == SynTree::R_array_type)
                array_type(scope,s);
            if( s->d_tok.d_type == SynTree::R_record_type)
                record_type(scope,s);
            if( s->d_tok.d_type == SynTree::R_set_type)
                set_type(scope,s);
            if( s->d_tok.d_type == SynTree::R_file_type)
                file_type(scope,s);
            if( s->d_tok.d_type == SynTree::R_class_type)
                class_type(scope,s);
        }
    }
    void array_type(Scope* scope, SynTree* st)
    {
        // TODO
    }
    void record_type(Scope* scope, SynTree* st)
    {
        // TODO
    }
    void set_type(Scope* scope, SynTree* st)
    {
        // TODO
    }
    void file_type(Scope* scope, SynTree* st)
    {
        // TODO
    }
    void class_type(Scope* scope, SynTree* st)
    {
        // TODO
    }
    void pointer_type(Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
            if( s->d_tok.d_type == SynTree::R_type_identifier)
                type_identifier(scope,s);
    }
    void variable_declaration_part( Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
            if( s->d_tok.d_type == SynTree::R_variable_declaration)
                variable_declaration(scope,s);
    }
    void variable_declaration(Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
        {
            if( s->d_tok.d_type == SynTree::R_identifier_list)
            {
                QList<Token> names = identifier_list(s);
                foreach( const Token& t, names )
                    addDecl(scope, t, Thing::Var);
            }
            if( s->d_tok.d_type == SynTree::R_type_)
                type_(scope,s);
        }
    }
    QList<Token> identifier_list(SynTree* st)
    {
        QList<Token> res;
        foreach( SynTree* s, st->d_children )
            if( s->d_tok.d_type == Tok_identifier)
                res.append(s->d_tok);
        return res;
    }

    void procedure_and_function_declaration_part( Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
        {
            if( s->d_tok.d_type == SynTree::R_procedure_declaration)
                procedure_declaration(scope,s);
            if( s->d_tok.d_type == SynTree::R_function_declaration)
                function_declaration(scope,s);
        }
    }
    void procedure_declaration(Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
        {
            if( s->d_tok.d_type == SynTree::R_procedure_heading)
                procedure_heading(scope,s);
            if( s->d_tok.d_type == SynTree::R_body_)
                body_(scope,s);
        }
    }
    void procedure_heading(Scope* scope, SynTree* st)
    {
        Token id = findIdent(st);
        Declaration* d = addDecl(scope,id, Thing::Proc);
        d->d_body = new Scope();
        d->d_body->d_owner = d;
        d->d_body->d_outer = scope;
        d->d_body->d_type = Thing::Body;
        foreach( SynTree* s, st->d_children )
            if( s->d_tok.d_type == SynTree::R_formal_parameter_list)
                formal_parameter_list(d->d_body, s);
    }
    void formal_parameter_list(Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
            if( s->d_tok.d_type == SynTree::R_formal_parameter_section)
                formal_parameter_section(scope,s);
    }
    void formal_parameter_section(Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
        {
            if( s->d_tok.d_type == SynTree::R_parameter_declaration)
                parameter_declaration(scope,s);
            if( s->d_tok.d_type == SynTree::R_procedure_heading)
                procedure_heading(scope,s);
            if( s->d_tok.d_type == SynTree::R_function_heading)
                function_heading(scope,s);
        }
    }
    void parameter_declaration(Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
        {
            if( s->d_tok.d_type == SynTree::R_identifier_list)
            {
                QList<Token> names = identifier_list(s);
                foreach( const Token& t, names )
                    addDecl(scope, t, Thing::Param);
            }
            if( s->d_tok.d_type == SynTree::R_type_identifier)
                type_identifier(scope,s);
        }
    }
    Symbol* addSym(Scope* scope, const Token& t)
    {
        Declaration* d = scope->findDecl(t.d_val);
        if( d )
        {
            Symbol* sy = new Symbol();
            sy->d_decl = d;
            sy->d_loc = t.toLoc();
            d_cf->d_syms.append(sy);
            if( sy->d_decl && sy->d_decl->isDeclaration() )
            {
                Declaration* d = static_cast<Declaration*>(sy->d_decl);
                d->d_refs[d_cf].append(sy);
            }
            return sy;
        }else
            return 0;
    }
    void type_identifier(Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
            if( s->d_tok.d_type == Tok_identifier)
                addSym(scope,s->d_tok);
    }
    void function_declaration(Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
        {
            if( s->d_tok.d_type == SynTree::R_function_heading)
                function_heading(scope,s);
            if( s->d_tok.d_type == SynTree::R_body_)
                body_(scope,s);
        }
    }
    Token findIdent(SynTree* st)
    {
        Token t;
        foreach( SynTree* s, st->d_children )
        {
            if( s->d_tok.d_type == Tok_identifier)
                t = s->d_tok;
        }
        return t;
    }
    void function_heading(Scope* scope, SynTree* st)
    {
        Token id = findIdent(st);
        Declaration* d = addDecl(scope,id, Thing::Func);
        d->d_body = new Scope();
        d->d_body->d_owner = d;
        d->d_body->d_outer = scope;
        d->d_body->d_type = Thing::Body;
        foreach( SynTree* s, st->d_children )
            if( s->d_tok.d_type == SynTree::R_formal_parameter_list)
                formal_parameter_list(d->d_body, s);
    }
    void body_(Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
        {
            if( s->d_tok.d_type == SynTree::R_block)
                block(scope,s);
            if( s->d_tok.d_type == SynTree::R_statement_part)
                statement_part(scope,s);
            if( s->d_tok.d_type == SynTree::R_constant)
                constant(scope,s);
        }
    }
    void statement_part( Scope* scope, SynTree* st)
    {
        if( !st->d_children.isEmpty() && st->d_children.first()->d_tok.d_type == SynTree::R_compound_statement )
            compound_statement(scope, st->d_children.first());
    }
    void statement_sequence(Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
            if( s->d_tok.d_type == SynTree::R_statement)
                statement(scope,s);
    }
    void statement(Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
        {
            if( s->d_tok.d_type == SynTree::R_simple_statement)
                simple_statement(scope,s);
            if( s->d_tok.d_type == SynTree::R_structured_statement)
                structured_statement(scope,s);
        }
    }
    void simple_statement(Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
            // goto statement is not interesting
            if( s->d_tok.d_type == SynTree::R_assigOrCall)
                assigOrCall(scope,s);
    }
    void structured_statement(Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
        {
            if( s->d_tok.d_type == SynTree::R_compound_statement)
                compound_statement(scope,s);
            if( s->d_tok.d_type == SynTree::R_repetitive_statement)
                repetitive_statement(scope,s);
            if( s->d_tok.d_type == SynTree::R_conditional_statement)
                conditional_statement(scope,s);
            if( s->d_tok.d_type == SynTree::R_with_statement)
                with_statement(scope,s);
        }
    }
    void compound_statement(Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
            if( s->d_tok.d_type == SynTree::R_statement_sequence)
                statement_sequence(scope,s);
    }
    void repetitive_statement(Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
        {
            if( s->d_tok.d_type == SynTree::R_while_statement)
                while_statement(scope,s);
            if( s->d_tok.d_type == SynTree::R_repeat_statement)
                repeat_statement(scope,s);
            if( s->d_tok.d_type == SynTree::R_for_statement)
                for_statement(scope,s);
        }
    }
    void while_statement(Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
        {
            if( s->d_tok.d_type == SynTree::R_expression)
                expression(scope,s);
            if( s->d_tok.d_type == SynTree::R_statement)
                statement(scope,s);
        }
    }
    void repeat_statement(Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
        {
            if( s->d_tok.d_type == SynTree::R_statement_sequence)
                statement_sequence(scope,s);
            if( s->d_tok.d_type == SynTree::R_expression)
                expression(scope,s);
        }
    }
    void for_statement(Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
        {
            if( s->d_tok.d_type == SynTree::R_variable_identifier && !s->d_children.isEmpty())
                addSym(scope,s->d_children.first()->d_tok);
            if( s->d_tok.d_type == SynTree::R_initial_value && !s->d_children.isEmpty())
                expression(scope,s->d_children.first());
            if( s->d_tok.d_type == SynTree::R_final_value && !s->d_children.isEmpty())
                expression(scope,s->d_children.first());
            if( s->d_tok.d_type == SynTree::R_statement)
                statement(scope,s);
        }
    }

    void conditional_statement(Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
        {
            if( s->d_tok.d_type == SynTree::R_if_statement)
                if_statement(scope,s);
            if( s->d_tok.d_type == SynTree::R_case_statement)
                case_statement(scope,s);
        }
    }
    void if_statement(Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
        {
            if( s->d_tok.d_type == SynTree::R_statement)
                statement(scope,s);
            if( s->d_tok.d_type == SynTree::R_expression)
                expression(scope,s);
        }
    }
    void case_statement(Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
        {
            if( s->d_tok.d_type == SynTree::R_case_limb)
                case_limb(scope,s);
            if( s->d_tok.d_type == SynTree::R_otherwise_clause)
                otherwise_clause(scope,s);
            if( s->d_tok.d_type == SynTree::R_expression)
                expression(scope,s);
        }
    }
    void case_limb(Scope* scope, SynTree* st)
    {
        // TODO
    }
    void otherwise_clause(Scope* scope, SynTree* st)
    {
        // TODO
    }
    void with_statement(Scope* scope, SynTree* st)
    {
        // TODO
    }
    void assigOrCall(Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
        {
            if( s->d_tok.d_type == SynTree::R_variable_reference)
                variable_reference(scope,s);
            if( s->d_tok.d_type == SynTree::R_expression)
                expression(scope,s);
        }
    }
    void constant(Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
        {
            if( s->d_tok.d_type == Tok_identifier)
                addSym(scope,s->d_tok);
            if( s->d_tok.d_type == SynTree::R_actual_parameter_list)
                actual_parameter_list(scope,s);
        }
    }
    void actual_parameter_list(Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
            if( s->d_tok.d_type == SynTree::R_actual_parameter)
                actual_parameter(scope,s);
    }
    void actual_parameter(Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
            if( s->d_tok.d_type == SynTree::R_expression)
                expression(scope,s);
    }
    void expression( Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
            if( s->d_tok.d_type == SynTree::R_simple_expression)
                simple_expression(scope,s);
    }
    void simple_expression(Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
            if( s->d_tok.d_type == SynTree::R_term)
                term(scope,s);
    }
    void term(Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
            if( s->d_tok.d_type == SynTree::R_factor)
                factor(scope,s);
    }
    void factor(Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
        {
            if( s->d_tok.d_type == SynTree::R_variable_reference)
                variable_reference(scope,s);
            if( s->d_tok.d_type == SynTree::R_actual_parameter_list)
                actual_parameter_list(scope,s);
            if( s->d_tok.d_type == SynTree::R_set_literal)
                set_literal(scope,s);
            if( s->d_tok.d_type == SynTree::R_expression)
                expression(scope,s);
            if( s->d_tok.d_type == Tok_identifier)
                addSym(scope,s->d_tok); // TODO: feed into qualifier
            if( s->d_tok.d_type == SynTree::R_factor)
                factor(scope,s);
            if( s->d_tok.d_type == SynTree::R_qualifier)
                qualifier(scope,s);
        }
    }
    void variable_reference(Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
        {
            if( s->d_tok.d_type == SynTree::R_variable_identifier && !s->d_children.isEmpty())
                addSym(scope,s->d_children.first()->d_tok); // TODO: feed into qualifier
            if( s->d_tok.d_type == SynTree::R_qualifier)
                qualifier(scope,s);
            if( s->d_tok.d_type == SynTree::R_actual_parameter_list)
                actual_parameter_list(scope,s);
        }
    }
    void set_literal(Scope* scope, SynTree* st)
    {

    }
    void qualifier(Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
        {
            if( s->d_tok.d_type == SynTree::R_index)
                index(scope,s);
            if( s->d_tok.d_type == SynTree::R_field_designator)
                field_designator(scope,s);
        }
    }
    void field_designator(Scope* scope, SynTree* st)
    {
        // TODO: resolve in correct scope (record)
    }
    void index(Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
            if( s->d_tok.d_type == SynTree::R_expression_list)
                expression_list(scope,s);
    }
    void expression_list(Scope* scope, SynTree* st)
    {
        foreach( SynTree* s, st->d_children )
            if( s->d_tok.d_type == SynTree::R_expression)
                expression(scope,s);
    }

protected:
};

CodeModel::CodeModel(QObject *parent) : QAbstractItemModel(parent),d_sloc(0)
{
    d_fs = new FileSystem(this);
}

bool CodeModel::load(const QString& rootDir)
{
    beginResetModel();
    d_root = Slot();
    d_top.clear();
    d_map1.clear();
    d_map2.clear();
    d_sloc = 0;
    d_fs->load(rootDir);
    QList<Slot*> fileSlots;
    fillFolders(&d_root,&d_fs->getRoot(), &d_top, fileSlots);
    foreach( Slot* s, fileSlots )
    {
        Q_ASSERT( s->d_thing && s->d_thing->d_type == Thing::File);
        CodeFile* f = static_cast<CodeFile*>(s->d_thing);
        Q_ASSERT( f->d_file );
        parseAndResolve(f);
        for( int i = 0; i < f->d_includes.size(); i++ )
            new Slot(s, f->d_includes[i]);
    }
    endResetModel();
    return true;
}

const Thing* CodeModel::getThing(const QModelIndex& index) const
{
    if( !index.isValid() )
        return 0;
    Slot* s = static_cast<Slot*>( index.internalPointer() );
    Q_ASSERT( s != 0 );
    return s->d_thing;
}

Symbol*CodeModel::findSymbolBySourcePos(const QString& path, int line, int col) const
{
    CodeFile* cf = d_map2.value(path);
    if( cf == 0 )
        return 0;
    foreach( Symbol* s, cf->d_syms )
    {
        if( s->d_decl && s->d_loc.d_row == line &&
                s->d_loc.d_col <= col && col < s->d_loc.d_col + s->d_decl->getLen() )
            return s;
    }

    return 0; // TODO
}

CodeFile*CodeModel::getCodeFile(const QString& path) const
{
    return d_map2.value(path);
}

QVariant CodeModel::data(const QModelIndex& index, int role) const
{
    Slot* s = static_cast<Slot*>( index.internalPointer() );
    Q_ASSERT( s != 0 );
    switch( role )
    {
    case Qt::DisplayRole:
        switch( s->d_thing->d_type )
        {
        case Thing::File:
            return static_cast<CodeFile*>(s->d_thing)->d_file->d_name;
        case Thing::Include:
            return static_cast<IncludeFile*>(s->d_thing)->d_file->d_name;
        case Thing::Folder:
            return static_cast<CodeFolder*>(s->d_thing)->d_dir->d_name;
        }
        break;
    case Qt::DecorationRole:
        switch( s->d_thing->d_type )
        {
        case Thing::File:
            return QPixmap(":/images/unit.png");
        case Thing::Include:
            return QPixmap(":/images/include.png");
        case Thing::Folder:
            return QPixmap(":/images/folder.png");
        }
        break;
    case Qt::ToolTipRole:
        switch( s->d_thing->d_type )
        {
        case Thing::File:
            {
                CodeFile* cf = static_cast<CodeFile*>(s->d_thing);
                return QString("<html><b>%1 %2</b><br>"
                               "<p>Logical path: %3</p>"
                               "<p>Real path: %4</p></html>")
                        .arg(cf->d_file->d_type == FileSystem::PascalUnit ? "Unit" : "Program")
                        .arg(cf->d_file->d_moduleName)
                        .arg(cf->d_file->getVirtualPath())
                        .arg(cf->d_file->d_realPath);
            }
        case Thing::Include:
            return static_cast<IncludeFile*>(s->d_thing)->d_file->d_realPath;
        }
        break;
    case Qt::FontRole:
        break;
    case Qt::ForegroundRole:
        break;
    }
    return QVariant();
}

QModelIndex CodeModel::index(int row, int column, const QModelIndex& parent) const
{
    const Slot* s = &d_root;
    if( parent.isValid() )
    {
        s = static_cast<Slot*>( parent.internalPointer() );
        Q_ASSERT( s != 0 );
    }
    if( row < s->d_children.size() && column < columnCount( parent ) )
        return createIndex( row, column, s->d_children[row] );
    else
        return QModelIndex();
}

QModelIndex CodeModel::parent(const QModelIndex& index) const
{
    if( index.isValid() )
    {
        Slot* s = static_cast<Slot*>( index.internalPointer() );
        Q_ASSERT( s != 0 );
        if( s->d_parent == &d_root )
            return QModelIndex();
        // else
        Q_ASSERT( s->d_parent != 0 );
        Q_ASSERT( s->d_parent->d_parent != 0 );
        return createIndex( s->d_parent->d_parent->d_children.indexOf( s->d_parent ), 0, s->d_parent );
    }else
        return QModelIndex();
}

int CodeModel::rowCount(const QModelIndex& parent) const
{
    if( parent.isValid() )
    {
        Slot* s = static_cast<Slot*>( parent.internalPointer() );
        Q_ASSERT( s != 0 );
        return s->d_children.size();
    }else
        return d_root.d_children.size();
}

Qt::ItemFlags CodeModel::flags(const QModelIndex& index) const
{
    Q_UNUSED(index)
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable; //  | Qt::ItemIsDragEnabled;
}

void CodeModel::parseAndResolve(CodeFile* file)
{
    if( file->d_file->d_parsed )
        return; // already done

    QByteArrayList usedNames = file->findUses();
    for( int i = 0; i < usedNames.size(); i++ )
    {
        const FileSystem::File* u = d_fs->findModule(file->d_file->d_dir,usedNames[i].toLower());
        if( u == 0 )
        {
            const QString line = tr("%1: cannot resolve referenced unit '%2'")
                    .arg( file->d_file->getVirtualPath(false) ).arg(usedNames[i].constData());
            qCritical() << line.toUtf8().constData();
        }else
        {
            CodeFile* tmp = d_map1.value(u);
            Q_ASSERT( tmp );
            file->d_import.append( tmp );
            parseAndResolve(tmp);
        }
    }

    const_cast<FileSystem::File*>(file->d_file)->d_parsed = true;
    PpLexer lex(d_fs);
    lex.reset(file->d_file->d_realPath);
    Parser p(&lex);
    p.RunParser();
    const int off = d_fs->getRootPath().size();
    if( !p.errors.isEmpty() )
    {
        foreach( const Parser::Error& e, p.errors )
        {
            const FileSystem::File* f = d_fs->findFile(e.path);
            const QString line = tr("%1:%2:%3: %4").arg( f ? f->getVirtualPath() : e.path.mid(off) ).arg(e.row)
                    .arg(e.col).arg(e.msg);
            qCritical() << line.toUtf8().constData();
        }

    }
    foreach( const PpLexer::Include& f, lex.getIncludes() )
    {
        IncludeFile* inc = new IncludeFile();
        inc->d_file = f.d_file;
        inc->d_loc = f.d_loc;
        inc->d_len = f.d_len;
        inc->d_includer = file;
        file->d_includes.append(inc);
    }
    d_sloc += lex.getSloc();

    CodeModelVisitor v(this);
    v.visit(file,&p.d_root);

    QCoreApplication::processEvents();
}

bool CodeModel::lessThan(const CodeModel::Slot* lhs, const CodeModel::Slot* rhs)
{
    if( lhs->d_thing == 0 || rhs->d_thing == 0 )
        return false;
    return lhs->d_thing->getName().compare(rhs->d_thing->getName(),Qt::CaseInsensitive) < 0;
}

void CodeModel::fillFolders(CodeModel::Slot* root, const FileSystem::Dir* super, CodeFolder* top, QList<CodeModel::Slot*>& fileSlots)
{
    for( int i = 0; i < super->d_subdirs.size(); i++ )
    {
        CodeFolder* f = new CodeFolder();
        f->d_dir = super->d_subdirs[i];
        top->d_subs.append(f);
        Slot* s = new Slot(root,f);
        fillFolders(s,super->d_subdirs[i],f,fileSlots);
    }
    for( int i = 0; i < super->d_files.size(); i++ )
    {
        if( super->d_files[i]->d_type == FileSystem::PascalProgram ||
                super->d_files[i]->d_type == FileSystem::PascalUnit )
        {
            CodeFile* f = new CodeFile();
            f->d_file = super->d_files[i];
            d_map1[f->d_file] = f;
            d_map2[f->d_file->d_realPath] = f;
            top->d_files.append(f);
            Slot* s = new Slot(root,f);
            fileSlots.append(s);
        }
    }
    std::sort( root->d_children.begin(), root->d_children.end(), lessThan );

}

QString CodeFile::getName() const
{
    return d_file->d_name;
}

QByteArrayList CodeFile::findUses() const
{
    QByteArrayList res;
    if( d_file == 0 || !(d_file->d_type == FileSystem::PascalProgram ||
                         d_file->d_type == FileSystem::PascalUnit) )
        return res;
    QFile f(d_file->d_realPath);
    f.open(QIODevice::ReadOnly);
    Lexer lex;
    lex.setStream(&f);
    Token t = lex.nextToken();
    while( t.isValid() )
    {
        switch( t.d_type )
        {
        case Tok_uses:
            t = lex.nextToken();
            while( t.isValid() && t.d_type != Tok_Semi )
            {
                if( t.d_type == Tok_Comma )
                {
                    t = lex.nextToken();
                    continue;
                }
                if( t.d_type == Tok_identifier )
                {
                    const QByteArray id = t.d_val;
                    t = lex.nextToken();
                    if( t.d_type == Tok_Slash )
                    {
                        t = lex.nextToken();
                        if( t.d_type == Tok_identifier ) // just to make sure
                        {
                            res.append( t.d_val );
                            t = lex.nextToken();
                        }
                    }else
                        res.append(id);
                }else
                    t = lex.nextToken();
            }
            return res;
        case Tok_label:
        case Tok_var:
        case Tok_const:
        case Tok_type:
        case Tok_procedure:
        case Tok_function:
        case Tok_implementation:
            return res;
        }
        t = lex.nextToken();
    }
    return res;
}

CodeFile::~CodeFile()
{
    if( d_impl )
        delete d_impl;
    if( d_intf )
        delete d_intf;
    for( int i = 0; i < d_syms.size(); i++ )
        delete d_syms[i];
    for( int i = 0; i < d_includes.size(); i++ )
        delete d_includes[i];
}

CodeFile*Scope::getCodeFile() const
{
    Q_ASSERT( d_owner != 0 );
    if( d_owner->d_type == Thing::File )
        return static_cast<CodeFile*>(d_owner);
    else if( d_owner->isDeclaration() )
    {
        Declaration* d = static_cast<Declaration*>(d_owner);
        Q_ASSERT( d->d_owner != 0 );
        return d->d_owner->getCodeFile();
    }else
        Q_ASSERT(false);
}

Declaration*Scope::findDecl(const QByteArray& name, bool withImports) const
{
#if 1
    // Cache effect is about 2-5%, too expensive for effect
    Declaration* d = d_cache.value(name);
    if( d )
        return d;
#endif
    foreach( Declaration* d, d_order )
    {
        if( d->d_name == name )
        {
            d_cache.insert(name,d);
            return d;
        }
    }
    if( d_outer )
        return d_outer->findDecl(name);

    return 0; // TODO: the following is still too slow (takes ~ 30% longer)
    if( withImports )
    {
        CodeFile* cf = getCodeFile();
        if( cf == 0 )
            return 0; // TODO: this happens, check
        foreach( CodeFile* imp, cf->d_import )
        {
            if( imp->d_intf )
            {
                Declaration* d = imp->d_intf->findDecl(name,false); // don't follow imports of imports
                if( d )
                {
                    d_cache.insert(name,d);
                    return d;
                }
            }
        }
    }
    return 0;
}

Scope::~Scope()
{
    for( int i = 0; i < d_order.size(); i++ )
        delete d_order[i];
}

QString Declaration::getFilePath() const
{
    CodeFile* file = getCodeFile();
    Q_ASSERT(file && file->d_file);
    return file->d_file->d_realPath;
}

QString Declaration::getName() const
{
    return d_name;
}

CodeFile*Declaration::getCodeFile() const
{
    Q_ASSERT( d_owner != 0 );
    return d_owner->getCodeFile();
}

Declaration::~Declaration()
{
    if( d_body )
        delete d_body;
}

QString CodeFolder::getName() const
{
    return d_dir->d_name;
}

void CodeFolder::clear()
{
    for( int i = 0; i < d_subs.size(); i++ )
        d_subs[i]->clear();
    d_subs.clear();
    for( int i = 0; i < d_files.size(); i++ )
        delete d_files[i];
    d_files.clear();
}

QString IncludeFile::getFilePath() const
{
    return d_file->d_realPath;
}

QString IncludeFile::getName() const
{
    return d_file->d_name;
}

QString Thing::getName() const
{
    return QString();
}

const char*Thing::typeName() const
{
    switch( d_type )
    {
    case Const:
        return "Const";
    case Type:
        return "Type";
    case Var:
        return "Var";
    case Func:
        return "Function";
    case Proc:
        return "Procedure";
    default:
        return ""; // TODO
    }
}

Thing::~Thing()
{

}
