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
#include "floatcol.h"
#include "intcol.h"
#include "boolcol.h"
#include "stringcol.h"
#include "column.h"
#include "string.h"
#include "fielder.h"
#include "schema.h"
#include "row.h"
#include "rower.h"
#include <iostream>
#include <thread>
#include "key.h"
#include "kvstore.h"
#include "reader.h"
#include "column_prov.h"
#include "writer.h"
#include "reader.h"
#include "parser.h"

using namespace std;

/** Represents a set of data */
class DataFrame : public Object {
public:
    Schema schema; // Cannot be changed
    Column **columns;
    int nrow;
    int ncol;

    /** Create a data frame with the same columns as the given df but with no rows or rownmaes */
    DataFrame(DataFrame &df) {
        int ncol = df.ncols();
        int nrow = df.nrows();
        schema = df.get_schema();
        columns = df.columns;
    }

    ~DataFrame() {
        for (int i = 0; i < ncol; i++) {
            delete columns[i];
        }
    }

    /** Create a data frame from a schema. All columns are created empty. */
    DataFrame(Schema &schema) {
        this->columns = new Column *[100 * 1000 * 1000];

        ncol = schema.width();

        this->schema = *new Schema(schema);

        nrow = 0;
        // set nrow to 0 but if schema comes in with nrow then use that
        //nrow = schema.nrow;
        // RESETS SCHEMA ROWS
        schema.nrow = 0;

        for (size_t i = 0; i < schema.ncol; i++) {
            char type = this->schema.col_type(i);
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

    /** Fill DataFrame from group 4500NE's sorer adapter */
    DataFrame(Provider::ColumnSet *data, size_t num_columns) {

        cout << "col set size: " << data->getColumn(0)->_length << endl;
        this->columns = new Column *[1000 * 1000 * 1000];
        this->schema = *new Schema();
        this->nrow = 0;
        this->ncol = 0;

        Column *working_col;

        // create columns and fill this df
        for (size_t i = 0; i < num_columns; i++) {
            // determine col type
            switch (data->getColumn(i)->getType()) {

                case Provider::ColumnType::BOOL :

                    working_col = new BoolColumn();
                    break;
                case Provider::ColumnType::FLOAT :

                    working_col = new FloatColumn();
                    break;
                case Provider::ColumnType::INTEGER :
                    working_col = new IntColumn();
                    break;
                case Provider::ColumnType::STRING :
                    working_col = new StringColumn();
                    break;
                case Provider::ColumnType::UNKNOWN :
                    exit(-1);
                    break;
            }
            // fill the specific typed column
            for (size_t j = 0; j < data->getColumn(i)->getLength(); j++) {
                switch (data->getColumn(i)->getType()) {
                    case Provider::ColumnType::BOOL :
                        if (checkColumnEntry(data->getColumn(i), j)) {
                            if (data->getColumn(i)->isEntryPresent(j)) {
                                working_col->push_back(
                                        dynamic_cast<Provider::BoolColumn *>(data->getColumn(i))->getEntry(j));
                            } else {
                                working_col->push_back(nullptr);
                            }
                        }
                        break;
                    case Provider::ColumnType::FLOAT :
                        if (checkColumnEntry(data->getColumn(i), j)) {
                            if (data->getColumn(i)->isEntryPresent(j)) {
                                working_col->push_back(
                                        dynamic_cast<Provider::FloatColumn *>(data->getColumn(i))->getEntry(j));
                            } else {
                                working_col->push_back(nullptr);
                            }
                        }
                        break;
                    case Provider::ColumnType::INTEGER :
                        if (checkColumnEntry(data->getColumn(i), j)) {
                            if (data->getColumn(i)->isEntryPresent(j)) {
                                working_col->push_back(
                                        dynamic_cast<Provider::IntegerColumn *>(data->getColumn(i))->getEntry(j));
                            } else {
                                working_col->push_back(nullptr);
                            }
                        }
                        break;
                    case Provider::ColumnType::STRING :
                        if (checkColumnEntry(data->getColumn(i), j)) {
                            if (data->getColumn(i)->isEntryPresent(j)) {
                                working_col->push_back(new String(
                                        dynamic_cast<Provider::StringColumn *>(data->getColumn(i))->getEntry(j)));
                            } else {
                                working_col->push_back(nullptr);
                            }
                        }
                        break;
                    case Provider::ColumnType::UNKNOWN:

                        break;
                }
            }


            // add this column to this dataframe
            this->add_column(working_col, nullptr);
        }


    }


    /**
     * Terminates if the given column is not large enough to have the given entry index.
     * @param col The column
     * @param which The entry index
     */
    bool checkColumnEntry(Provider::BaseColumn *col, size_t which) {
        if (which >= col->getLength()) {
            return false;
        }
        return true;
    }

    /** Returns the dataframe's schema. Modifying the schema after a dataframe
      * has been created in undefined. */
    Schema &get_schema() {
        return schema;
    }

    /** Adds a column this dataframe, updates the schema, the new column
      * is external, and appears as the last column of the dataframe, the
      * name is optional and external. A nullptr colum is undefined. */
    void add_column(Column *col, String *name) {
        if (col == nullptr) {
            exit(1);
        } else {
            columns[ncol] = col;
            schema.add_column(col->get_type(), name);
            if (col->size() > nrow) {
                this->nrow = col->size();
                schema.nrow = col->size();
            }
            ncol++;
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

    /** Return the offset of the given column name or -1 if no such col. */
    int get_col(String &col) {
        return schema.col_idx(col.c_str());
    }

    /** Return the offset of the given row name or -1 if no such row. */
    int get_row(String &col) {
        return schema.row_idx(col.c_str());
    }

    Row* get_row(size_t i) {

        if (i < 0 || i >= this->nrows()) {
            //cout << "returning nullptr" << endl;
            return nullptr;
        }

//        // run through columns at index i to build row
        Row* build_row = new Row(this->schema);
//        for (size_t col_idx = 0; this->ncol; col_idx++) {
//            switch (this->get_schema().col_type(col_idx)) {
//                case 'B':
//                    build_row->set(i,this->get_bool(col_idx,i));
//                    break;
//                case 'I':
//                    build_row->set(i,this->get_int(col_idx,i));
//                    break;
//                case 'F':
//                    build_row->set(i,this->get_float(col_idx,i));
//                    break;
//                case 'S':
//                    build_row->set(i,this->get_string(col_idx,i));
//                    break;
//            }
//        }

        this->fill_row(i,*build_row);
        //cout << "row built = ";
        build_row->printRow();
        //cout << endl;
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
        for (size_t i = 0; i < ncol; i++) {
            switch (columns[i]->get_type()) {
                case 'F':
                    row.set(i, columns[i]->as_float()->get(idx));
                    break;
                case 'B':
                    row.set(i, (bool)*columns[i]->as_bool()->get(idx));
                    break;
                case 'I':
                    row.set(i, (int)*columns[i]->as_int()->get(idx));
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
        row.set_idx(nrow);
        this->nrow++;
        schema.nrow++;
        for (size_t i = 0; i < ncol; i++) {
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
    size_t nrows() {
        //return schema.length();

        return nrow;
    }

    /** The number of columns in the dataframe.*/
    size_t ncols() {
        return schema.width();
    }

    /** Visit rows in order */
    void map(Rower &r) {
        //cout << "schema: " << this->get_schema().types->c_str() << endl;
        for (size_t i = 0; i < this->nrows(); i++) {
            Row *row = new Row(this->schema);
//            for (size_t j = 0; j < this->ncols(); j++) {
//                switch (row->col_type(j)) {
//                    case 'I':
//                        //cout << "setting int" << endl;
//                        row->set(j, this->columns[j]->as_int()->get(i));
//                        break;
//                    case 'B':
//                        row->set(j, this->columns[j]->as_bool()->get(i));
//                        break;
//                    case 'S':
//                        row->set(j, this->columns[j]->as_string()->get(i));
//                        break;
//                    case 'F':
//                        row->set(j, this->columns[j]->as_float()->get(i));
//                        break;
//                }
//            }
            fill_row(i, *row);
            r.accept(*row);
        }
    }

    /** Visits the rows in order on THIS node */
    void map(Reader* r) {
        //cout << "bad map" << endl;
        int completed = 0;
        cout << "local map: nrows = " << this->nrows() << endl;
        for (size_t i = 0; i < this->nrows(); i++) {
            Row *row = new Row(this->schema);
            this->fill_row(i,*row);
            completed++;
            r->visit(*row);
//            //cout << completed << " " << "row visited | ";
//            row->printRow();
//            //cout << endl;
        }
    }

    /** Visits the rows in order on THIS node */
    void local_map(Reader* r) {
        //cout << "bad map" << endl;
        map(r);
    }

    /** Visits the rows in order on THIS node */
    void local_map(Adder &r) {
        int completed = 0;
        //cout << "schema" << schema.types->c_str() << endl;
        //cout << "local map: nrows = " << this->nrows() << endl;
        for (size_t i = 0; i < this->nrows(); i++) {
            Row *row = new Row(this->schema);
            this->fill_row(i, *row);
            completed++;
//            assert(row->col_type(0)=="S");
//            assert(row->col_type(1)=="I");
            //cout << "row->coltype 0 " << row->col_type(0) << endl;
            //cout << "row->coltype 1 " << row->col_type(1) << endl;
            r.visit(*row);
            //cout << completed << " " << "row visited | ";
            //row->printRow();
            //cout << endl;
        }
    }

    /** Applies the given Adder to each Row in this DataFrame **/
    void map(Adder &r) {
        local_map(r);
    }

    /** Create a new dataframe, constructed from rows for which the given Rower
      * returned true from its accept method. */
    DataFrame *filter(Rower &r) {
        DataFrame *d = new DataFrame(this->get_schema());
        for (size_t i = 0; i < this->nrows(); i++) {
            Row *row = new Row(this->schema);
            for (size_t j = 0; j < this->ncols(); j++) {
                switch (row->col_type(j)) {
                    case 'I':
                        row->set(j, this->columns[j]->as_int()->get(i));
                        break;
                    case 'B':
                        row->set(j, this->columns[j]->as_bool()->get(i));
                        break;
                    case 'S':
                        row->set(j, this->columns[j]->as_string()->get(i));
                        break;
                    case 'F':
                        row->set(j, this->columns[j]->as_float()->get(i));
                        break;
                }
            }
            if (r.accept(*row)) {
                d->add_row(*row);
            }
        }
        return d;
    }

    /** Print the dataframe in SoR format to standard output. */
    void print() {
        for (size_t i = 0; i < ncol; i++) {
            for (size_t j = 0; j < nrow; j++) {
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

    /**
     * Contructs a DataFrame from the given array of doubles and associates the given Key with the DataFrame in the given KVStore
     */
    static DataFrame *fromArray(Key *key, KVStore *kv, size_t sz, double *vals) {
        DataFrame *df = new DataFrame(*new Schema("F"));
        for (int i = 0; i < sz; i++) {
            df->columns[0]->push_back((float) vals[i]);
        }
        kv->put(key, df);
        return df;
    }

    /**
     * Contructs a DataFrame of the given schema from the given FileReader and puts it in the KVStore at the given Key
     */
    static DataFrame *fromVisitor(Key key, KVStore kv, const char *schema, Writer* w) {
        cout << "in fromVisitor" << endl;
        cout << key
        DataFrame *df = new DataFrame(*new Schema(schema));
        cout << "made df" << endl;
        while (!w->done()) {
//            cout << "making row" << endl;
            Row *r = new Row(*new Schema(schema));
            w->visit(*r);
            df->add_row(*r);
            //cout << "ROW: " << r->get_string(0)->c_str() << endl;
        }
        cout << "putting in kv under key " << key.name->cstr_ << "df size - " << df->nrows() << endl;
        Key* key2;
        *key2 = key;
        kv.put(key2, df);
        if (key.name->equals(new String("users-1-0"))) {
            //cout << "kv.keys[1] = " << kv->keys[1]->name->c_str() << endl;
        }
        return df;
    }

    /**
 * Contructs a DataFrame of the given schema from the given FileReader and puts it in the KVStore at the given Key
 */
    static DataFrame *fromVisitor(Key *key, KVStore *kv, char *schema, FileReader w) {
        DataFrame *df = new DataFrame(*new Schema(schema));
        while (!w.done()) {
            Row *r = new Row(*new Schema(schema));
            w.visit(*r);
            df->add_row(*r);
            //cout << "ROW: " << r->get_string(0)->c_str() << endl;
        }
        kv->put(key, df);
        return df;
    }

    /**
     * Contructs a DataFrame of the given schema from the given FileReader and puts it in the KVStore at the given Key
     */
    static DataFrame *fromVisitor(Key *key, KVStore *kv, const char *schema, FileReader w) {
        DataFrame *df = new DataFrame(*new Schema(schema));
        while (!w.done()) {
            Row *r = new Row(*new Schema(schema));
            w.visit(*r);
            df->add_row(*r);
            //cout << "ROW: " << r->get_string(0)->c_str() << endl;
        }
        kv->put(key, df);
        return df;
    }

    /** Returns a section of this DataFrame as a new DataFrame **/
    DataFrame *chunk(size_t chunk_select) {

        int start_row = chunk_select * arg.rows_per_chunk;
        //cout << "start row: " << start_row << endl;
        //cout << "chunk select " << chunk_select << endl;
        DataFrame *df = new DataFrame(this->schema);

        for (size_t i = start_row; i < start_row + arg.rows_per_chunk; i++) {
            if (i >= nrow) {
                return df;
            } else {
                Row *r = new Row(this->schema);
                this->fill_row(i, *r);
                df->add_row(*r);
            }
        }
        return df;
    };

    /**
     * Contructs a DataFrame of the given schema from the given Summer and puts it in the KVStore at the given Key
     */
    static DataFrame *fromVisitor(Key *key, KVStore *kv, char *schema, Summer w) {
        DataFrame *df = new DataFrame(*new Schema(schema));
        while (!w.done()) {
            Row *r = new Row(*new Schema(schema));
            w.visit(*r);
            df->add_row(*r);
        }
        kv->put(key, df);
        return df;
    }

    /**
     * Returns the double at the given column and row in this DataFrame
     */
    float get_double(int col, int row) {
        return *this->columns[col]->as_float()->get(row);
    }

    /**
     * Contructs a DataFrame from the size_t and associates the given Key with the DataFrame in the given KVStore
     */
    static DataFrame *fromScalarInt(Key *key, KVStore *kv, size_t scalar) {
        cout << "Creating df " << endl;
        DataFrame *df = new DataFrame(*new Schema("I"));
        cout << "pushing back" << endl;
        df->columns[0]->push_back((int) scalar);
        if (df->nrow < df->columns[0]->size()){
            df->nrow = df->columns[0]->size();
        }
        cout << "putting in kv store: " << key->name->c_str()  << "size of df" << df->nrows() << endl;
        kv->put(key, df);
        cout << "done in fromScalarInt" << endl;
        return df;
    }

    static size_t get_file_size(FILE* p_file) // path to file
    {
        fseek(p_file,0,SEEK_END);
        size_t size = ftell(p_file);
        fclose(p_file);
        return size;
    }

    static DataFrame* fromFile(const char* filep, Key* key, KVStore* kv) {
        cout << "in from file: " << filep << endl;
        FILE* file = fopen(filep, "rb");
        FILE* file_dup = fopen(filep, "rb");
        cout << "fopen file null?: " << (file == nullptr) << endl;
        size_t file_size = get_file_size(file_dup);
        cout << "size of file calced " << file_size << endl;

        cout << "file opened" << endl;
        SorParser parser(file, (size_t)0, (size_t)file_size, (size_t)file_size);
        cout << "parser created" << endl;
        parser.guessSchema();
        cout << "schema guessed" << endl;
        parser.parseFile();
        cout << "file parsed" << endl;
        DataFrame* d = new DataFrame(parser.getColumnSet(), parser._num_columns);
        cout << "data frame created of SIZE " << d->nrows() << endl;
        return d;
    }

    /**
     * Adds chunk dataframe passed in to this dataframe
     */
     DataFrame* append_chunk(DataFrame* df) {
         for (size_t r = 0; r < df->nrows(); r++) {
             this->add_row(*df->get_row(r));
         }

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
    bool* vals_;  // owned; data
    size_t size_; // number of elements

    /** Creates a set of the same size as the dataframe. */
    Set(DataFrame* df) : Set(df->nrows()) {}

    /** Creates a set of the given size. */
    Set(size_t sz) {
        vals_ = new bool[sz];
        size_ = sz;
        cout << "creating set" << endl;
        for(size_t i = 0; i < size_; i++) {
            vals_[i] = false;
        }
    }

    ~Set() { //delete[] vals_;
        }

    /** Add idx to the set. If idx is out of bound, ignore it.  Out of bound
     *  values can occur if there are references to pids or uids in commits
     *  that did not appear in projects or users.
     */
    void set(size_t idx) {
        if (idx >= size_ ) return; // ignoring out of bound writes
        vals_[idx] = true;
    }

    /** Is idx in the set?  See comment for set(). */
    bool test(size_t idx) {
        if (idx >= size_) return true; // ignoring out of bound reads
        return vals_[idx];
    }

    size_t size() { return size_; }

    /** Performs set union in place. */
    void union_(Set& from) {
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
class SetWriter: public Writer {
public:
    Set& set_; // set to read from
    int i_ = 0;  // position in set

    SetWriter(Set& set) : set_(set){
    }

    /** Skip over false values and stop when the entire set has been seen */
    virtual bool done() override {
        while (i_ < set_.size_ && set_.test(i_) == false) ++i_;
//        cout << "i " << i_ << "size " << set_.size_ << endl;
        return i_ == set_.size_;
    }

    virtual void visit(Row & row) override { row.set(0, i_++); }
};


/*******************************************************************************
 * A SetUpdater is a reader that gets the first column of the data frame and
 * sets the corresponding value in the given set.
 ******************************************************************************/
class SetUpdater : public Reader {
public:
    Set& set_; // set to update

    SetUpdater(Set& set): set_(set) {}

    /** Assume a row with at least one column of type I. Assumes that there
     * are no missing. Reads the value and sets the corresponding position.
     * The return value is irrelevant here. */
    bool visit(Row & row) { set_.set(row.get_int(0));  return false; }

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
    Set& uSet; // set of collaborator
    Set& pSet; // set of projects of collaborators
    Set newProjects;  // newly tagged collaborator projects

    ProjectsTagger(Set& uSet, Set& pSet, DataFrame* proj):
            uSet(uSet), pSet(pSet), newProjects(proj) {}

    /** The data frame must have at least two integer columns. The newProject
     * set keeps track of projects that were newly tagged (they will have to
     * be communicated to other nodes). */
    bool visit(Row & row) override {
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
            pSet(pSet), uSet(uSet), newUsers(users->nrows()) {}

    //Kate did some stuff
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
