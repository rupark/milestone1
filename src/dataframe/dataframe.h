/****************************************************************************
 * DataFrame::
 *
 * A DataFrame is table composed of columns of equal length. Each column
 * holds values of the same type (I, S, B, F). A dataframe has a schema that
 * describes it.
 */
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include "../column/floatcol.h"
#include "../column/intcol.h"
#include "../column/boolcol.h"
#include "../column/stringcol.h"
#include "../column/column.h"
#include "../wrappers/string.h"
#include "../fielder.h"
#include "schema.h"
#include "row.h"
#include "../rower.h"
#include <iostream>
#include <thread>
#include "../reader.h"
#include "../writer.h"
#include "../reader.h"
#include "../array.h"

using namespace std;

/** Represents a set of data */
class DataFrame : public Object {
public:
    Schema *schema; // Cannot be changed
    Column **columns;

    /** Create a data frame with the same columns as the given df but with no rows or rownmaes */
    DataFrame(DataFrame &df) {
        int ncol = df.get_num_cols();
        int nrow = df.get_num_rows();
        schema = df.get_schema();
        columns = df.columns;

    }

    ~DataFrame() {
        for (int i = 0; i < this->schema->get_num_cols(); i++) {
            if (columns[i] != nullptr) {
                delete columns[i];
            }
        }
        delete[] columns;
        delete schema;
    }

    /** Create a data frame from a schema-> All columns are created empty. */
    DataFrame(Schema &schema) {
        this->columns = new Column *[10];
        this->schema = new Schema(schema);
        this->schema->nrow = 0;

        for (size_t i = 0; i < this->get_num_cols(); i++) {
            char type = this->schema->col_type(i);
            switch (type) {
                case 'F':
                    columns[i] = new FloatColumn();
                    break;
                case 'B':
                    columns[i] = new BoolColumn();
                    break;
                case 'I':
                    columns[i] = new IntColumn();
                    break;
                case 'S':
                    columns[i] = new StringColumn();
                    break;
            }
        }
    }

    /** Returns the dataframe's schema-> Modifying the schema after a dataframe
      * has been created in undefined. */
    Schema *get_schema() {
        return schema;
    }

    /** Adds a column this dataframe, updates the schema, the new column
      * is external, and appears as the last column of the dataframe, the
      * name is optional and external. A nullptr colum is undefined. */
    void add_column(Column *col) {
        if (col == nullptr) {
            exit(1);
        } else {
            columns[this->get_num_cols()] = col;
            schema->add_column(col->get_type());
            if (col->size() > schema->get_num_rows()) {
                schema->nrow = col->size();
            }
        }
    }

    /** Return the value at the given column and row. Accessing rows or
     *  columns out of bounds, or request the wrong type is undefined.*/
    int get_int(size_t col, size_t row) {
        return *columns[col]->as_int()->get(row);
    }

    bool get_bool(size_t col, size_t row) {
        return *columns[col]->as_bool()->get(row);
    }

    float get_float(size_t col, size_t row) {
        return *columns[col]->as_float()->get(row);
    }

    String *get_string(size_t col, size_t row) {
        return columns[col]->as_string()->get(row);
    }

    Row *get_row(size_t i) {
        if (i < 0 || i >= this->get_num_rows()) {
            return nullptr;
        }
        Row *build_row = new Row(this->schema);
        this->fill_row(i, *build_row);
        return build_row;
    }

    /** Set the value at the given column and row to the given value.
      * If the column is not  of the right type or the indices are out of
      * bound, the result is undefined. */
    void set(size_t col, size_t row, int val) {
        columns[col]->as_int()->set(row, &val);
    }

    void set(size_t col, size_t row, bool val) {
        columns[col]->as_bool()->set(row, &val);
    }

    void set(size_t col, size_t row, float val) {
        columns[col]->as_float()->set(row, &val);
    }

    void set(size_t col, size_t row, String *val) {
        columns[col]->as_string()->set(row, val);
    }

    /** Set the fields of the given row object with values from the columns at
      * the given offset.  If the row is not form the same schema as the
      * dataframe, results are undefined.
      */
    void fill_row(size_t idx, Row &row) {
        for (size_t i = 0; i < this->get_num_cols(); i++) {
            switch (columns[i]->get_type()) {
                case 'F':
                    row.set(i, columns[i]->as_float()->get(idx));
                    break;
                case 'B':
                    row.set(i, (bool) *columns[i]->as_bool()->get(idx));
                    break;
                case 'I':
                    row.set(i, (int) *columns[i]->as_int()->get(idx));
                    break;
                case 'S':
                    row.set(i, columns[i]->as_string()->get(idx));
                    break;
            }
        }
    }

    /** Add a row at the end of this dataframe. The row is expected to have
     *  the right schema and be filled with values, otherwise undedined.  */
    void add_row(Row &row) {
        row.set_idx(schema->get_num_rows());
        schema->add_row();
        for (size_t i = 0; i < get_num_cols(); i++) {
            switch (columns[i]->get_type()) {
                case 'F':
                    columns[i]->push_back(row.get_float(i));
                    break;
                case 'B':
                    columns[i]->push_back(row.get_bool(i));
                    break;
                case 'I':
                    this->columns[i]->push_back(row.get_int(i));
                    break;
                case 'S':
                    columns[i]->push_back(row.get_string(i));
                    break;
            }
        }
    }

    /** The number of rows in the dataframe. */
    size_t get_num_rows() {
        if (get_num_cols() == 0) {
            return 0;
        }

        size_t num_rows = columns[0]->size();
        for (size_t i = 0; i < get_num_cols(); i++) {
            if (columns[i]->size() > num_rows) {
                num_rows = columns[i]->size();
            }
        }
        this->get_schema()->nrow = num_rows;

        return schema->get_num_rows();
    }

    /** The number of columns in the dataframe.*/
    size_t get_num_cols() {
        return schema->get_num_cols();
    }

    /** Visits the rows in order on THIS node */
    void map(Reader *r) {
        int completed = 0;

        for (size_t i = 0; i < this->get_num_rows(); i++) {
            Row *row = new Row(this->schema);
            this->fill_row(i, *row);
            completed++;
            r->visit(*row);
            delete row;
        }

    }

    /** Visits the rows in order on THIS node */
    void map(Writer *r) {
        int completed = 0;

        for (size_t i = 0; i < this->get_num_rows(); i++) {
            Row *row = new Row(this->schema);
            this->fill_row(i, *row);
            completed++;
            r->visit(*row);
            delete row;
        }

    }

    /** Print the dataframe in SoR format to standard output. */
    void print() {
        for (size_t i = 0; i < get_num_cols(); i++) {
            for (size_t j = 0; j < get_num_rows(); j++) {
                switch (columns[i]->get_type()) {
                    case 'F':
                        cout << "<" << *columns[i]->as_float()->get(j) << ">";
                        break;
                    case 'B':
                        cout << "<" << *columns[i]->as_bool()->get(j) << ">";
                        break;
                    case 'I':
                        cout << "<" << *columns[i]->as_int()->get(j) << ">";
                        break;
                    case 'S':
                        cout << "<" << columns[i]->as_string()->get(j)->cstr_ << ">";
                        break;
                }
            }
            cout << endl;
        }
    }

    /** Returns a section of this DataFrame as a new DataFrame **/
    DataFrame *chunk(size_t chunk_select) {

        int start_row = chunk_select * arg.rows_per_chunk;
        DataFrame *df = new DataFrame(*this->schema);

        for (size_t i = start_row; i < start_row + arg.rows_per_chunk; i++) {
            if (i >= this->get_num_rows()) {
                return df;
            } else {
                Row *r = new Row(this->schema);
                this->fill_row(i, *r);
                df->add_row(*r);
                delete r;
            }
        }
        return df;
    }

    /**
     * Returns the double at the given column and row in this DataFrame
     */
    float get_double(int col, int row) {
        return *this->columns[col]->as_float()->get(row);
    }

    /**
     * Adds chunk dataframe passed in to this dataframe
     */
    DataFrame *append_chunk(DataFrame *df) {
        for (size_t r = 0; r < df->get_num_rows(); r++) {
            Row *r2 = df->get_row(r);
            this->add_row(*r2);
            delete r2;
        }
        delete df;

        return this;
    }

};


/**************************************************************************
 * A bit set contains size() booleans that are initialize to false and can
 * be set to true with the set() method. The test() method returns the
 * value. Does not grow.
 ************************************************************************/
class Set {
public:
    bool *vals_;  // owned; data
    size_t size_; // number of elements

    /** Creates a set of the same size as the dataframe. */
    Set(DataFrame *df) {
        vals_ = new bool[df->get_num_rows()];
        size_ = df->get_num_rows();
        for (size_t i = 0; i < size_; i++) {
            vals_[i] = false;
        }
    }

    /** Creates a set of the given size. */
    Set(size_t sz) {
        vals_ = new bool[sz];
        size_ = sz;
        for (size_t i = 0; i < size_; i++) {
            vals_[i] = false;
        }
    }

    ~Set() {
        delete[] vals_;
    }

    /** Add idx to the set. If idx is out of bound, ignore it.  Out of bound
     *  values can occur if there are references to pids or uids in commits
     *  that did not appear in projects or users.
     */
    void set(size_t idx) {
        if (idx >= size_) return; // ignoring out of bound writes
        vals_[idx] = true;
    }

    /** Is idx in the set?  See comment for set(). */
    bool test(size_t idx) {
        if (idx >= size_) return true; // ignoring out of bound reads
        return vals_[idx];
    }

    size_t size() { return size_; }

    size_t num_true() {
        size_t size = 0;
        for (size_t i = 0; i < size_; i++) {
            if (vals_[i] == true) {
                size++;
            }
        }
        return size;
    }

    /** Performs set union in place. */
    void union_(Set &from) {
        for (size_t i = 0; i < from.size_; i++)
            if (from.test(i))
                set(i);
    }
};

/*****************************************************************************
 * A SetWriter copies all the values present in the set into a one-column
 * dataframe. The data contains all the values in the set. The dataframe has
 * at least one integer column.
 ****************************************************************************/
class SetWriter : public Writer {
public:
    Set &set_; // set to read from
    int i_ = 0;  // position in set

    SetWriter(Set &set) : set_(set) {
    }

    /** Skip over false values and stop when the entire set has been seen */
    virtual bool done() override {
        while (i_ < set_.size_ && set_.test(i_) == false) ++i_;
        return i_ == set_.size_;
    }

    virtual void visit(Row &row) override { row.set(0, i_++); }
};


/*******************************************************************************
 * A SetUpdater is a reader that gets the first column of the data frame and
 * sets the corresponding value in the given set.
 ******************************************************************************/
class SetUpdater : public Reader {
public:
    Set &set_; // set to update

    SetUpdater(Set &set) : set_(set) {}

    /** Assume a row with at least one column of type I. Assumes that there
     * are no missing. Reads the value and sets the corresponding position.
     * The return value is irrelevant here. */
    bool visit(Row &row) {
        set_.set(row.get_int(0));
        return false;
    }

};


/***************************************************************************
 * The ProjectTagger is a reader that is mapped over commits, and marks all
 * of the projects to which a collaborator of Linus committed as an author.
 * The commit dataframe has the form:
 *    pid x uid x uid
 * where the pid is the identifier of a project and the uids are the
 * identifiers of the author and committer. If the author is a collaborator
 * of Linus, then the project is added to the set. If the project was
 * already tagged then it is not added to the set of newProjects.
 *************************************************************************/
class ProjectsTagger : public Reader {
public:
    Set &uSet; // set of collaborator
    Set &pSet; // set of projects of collaborators
    Set newProjects;  // newly tagged collaborator projects

    ProjectsTagger(Set &uSet, Set &pSet, DataFrame *proj) :
            uSet(uSet), pSet(pSet), newProjects(proj) {}

    /** The data frame must have at least two integer columns. The newProject
     * set keeps track of projects that were newly tagged (they will have to
     * be communicated to other nodes). */
    bool visit(Row &row) override {
        int pid = row.get_int(0);
        int uid = row.get_int(1);
        if (uSet.test(uid)) {
            if (!pSet.test(pid)) {
                pSet.set(pid);
                newProjects.set(pid);
                return true;
            }
            return false;
        }
    }
};

/***************************************************************************
 * The UserTagger is a reader that is mapped over commits, and marks all of
 * the users which commmitted to a project to which a collaborator of Linus
 * also committed as an author. The commit dataframe has the form:
 *    pid x uid x uid
 * where the pid is the idefntifier of a project and the uids are the
 * identifiers of the author and committer.
 *************************************************************************/
class UsersTagger : public Reader {
public:
    Set &pSet;
    Set &uSet;
    Set newUsers;

    UsersTagger(Set &pSet, Set &uSet, DataFrame *users) :
            pSet(pSet), uSet(uSet), newUsers(users->get_num_rows()) {
    }

    bool visit(Row &row) override {
        int pid = row.get_int(0);
        int uid = row.get_int(1);
        if (pSet.test(pid)) {
            if (!uSet.test(uid)) {
                uSet.set(uid);
                newUsers.set(uid);
                return true;
            }
            return false;
        }
    }
};
