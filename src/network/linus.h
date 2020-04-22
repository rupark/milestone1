//
// Created by Kate Rupar on 4/9/20.
#pragma once

#include "application.h"
#include "../dataframe.h"
#include "../key.h"
#include "../reader.h"
#include "../SImap.h"
#include "../writer.h"
#include "network.h"
#include <iostream>
#include "../parser.h"

using namespace std;

/*************************************************************************
 * This computes the collaborators of Linus Torvalds.
 * is the linus example using the adapter.  And slightly revised
 *   algorithm that only ever trades the deltas.
 **************************************************************************/
class Linus : public Application {
public:
    int DEGREES = 4;  // How many degrees of separation form linus?
    int LINUS = 4967;   // The uid of Linus (offset in the user df)
    bool subset = false;

    const char *PROJ = (subset ? "datasets/projects_subset.ltgt" : "datasets/projects.ltgt");
    const char *USER = (subset ? "datasets/users_subset.ltgt" : "datasets/users.ltgt");
    const char *COMM = (subset ? "datasets/commits_subset.ltgt" : "datasets/commits.ltgt");

    DataFrame *projects; //  pid x project name
    DataFrame *users;  // uid x user name
    DataFrame *commits;  // pid x uid x uid
    Set *uSet; // Linus' collaborators
    Set *pSet; // projects of collaborators

    Linus(size_t idx, NetworkIP &net) : Application(idx, net) {}

    ~Linus() {
        cout << "in linus des" << endl;
        cout << "3" << endl;
        delete projects;
        cout << "4" << endl;
        delete users;
        cout << "5" << endl;
        delete commits;
        cout << "6" << endl;
        delete uSet;
        cout << "7" << endl;
        delete pSet;
        cout << "8" << endl;
        //delete kv;
        //delete net;
    }

    /** Compute DEGREES of Linus.  */
    void run_() override {
        readInput();
        cout << "READING INPUT" << endl;

        for (size_t i = 0; i < DEGREES; i++) step(i);
    }

    size_t get_file_size(FILE *p_file) // path to file
    {
        fseek(p_file, 0, SEEK_END);
        size_t size = ftell(p_file);
        fclose(p_file);
        return size;
    }

    DataFrame *readDataFrameFromFile(const char *filep) {
        cout << "in from file: " << filep << endl;
        FILE *file = fopen(filep, "rb");
        FILE *file_dup = fopen(filep, "rb");
        cout << "fopen file null?: " << (file == nullptr) << endl;
        size_t file_size = get_file_size(file_dup);
        cout << "size of file calced " << file_size << endl;
        cout << "file opened" << endl;


        ///////////////////////////////////////
        SorParser *parser = new SorParser(file, (size_t) 0, (size_t) file_size, (size_t) file_size);
        cout << "parser created" << endl;
        parser->guessSchema();
        cout << "schema guessed" << endl;
        try {
            parser->parseFile();
        } catch (const std::exception &e) {
            cout << "PARSE FILE: " << e.what() << endl;
        }

        ///////////////////////////////////////
        fclose(file);
        //  fclose(file_dup);

        // delete[] file;
        //delete[] file_dup;
        cout << "file parsed" << endl;
//        DataFrame* d = new DataFrame(parser->getColumnSet(), parser->_num_columns);
        DataFrame *d = parser->parsed_df;
        cout << "data frame created of SIZE " << d->get_num_rows() << endl;
        delete parser;

        return d;
    }

    /** Node 0 reads three files, cointainng projects, users and commits, and
     *  creates thre dataframes. All other nodes wait and load the three
     *  dataframes. Once we know the size of users and projects, we create
     *  sets of each (uSet and pSet). We also output a data frame with a the
     *  'tagged' users. At this point the dataframe consists of only
     *  Linus. **/
    void readInput() {
        Key *pK = new Key("projs");
        Key *uK = new Key("usrs");
        Key *cK = new Key("comts");
        if (this_node() == 0) {
            // We never put these dfs in the store???
            cout << "Reading..." << endl;
//            commits = DataFrame::fromFile(COMM, cK, kv);
            commits = readDataFrameFromFile(COMM);
            cout << "    " << commits->get_num_rows() << " commits" << endl;
//            projects = DataFrame::fromFile(PROJ, pK, kv);
            projects = readDataFrameFromFile(PROJ);
            cout << "    " << projects->get_num_rows() << " projects" << endl;
//            users = DataFrame::fromFile(USER, uK, kv);
            users = readDataFrameFromFile(USER);
            cout << "    " << users->get_num_rows() << " users" << endl;
            // This dataframe contains the id of Linus.
            //delete
            Key *key = new Key("users-0-0");
            fromScalarInt(key, kv, LINUS);
        } else {
            projects = kv->get(pK);
            users = kv->get(uK);
            commits = kv->get(cK);
        }
        cout << "making sets" << endl;
        uSet = new Set(users);
        cout << "made a set" << endl;
        pSet = new Set(projects);
        cout << "done making sets" << endl;
    }

    /**
     * Contructs a DataFrame from the given array of doubles and associates the given Key with the DataFrame in the given KVStore
     */
    static void *fromArray(Key *key, KVStore *kv, size_t sz, double *vals) {
        Schema *s = new Schema("F");
        DataFrame *df = new DataFrame(*s);
        delete s;
        for (int i = 0; i < sz; i++) {
            df->columns[0]->push_back((float) vals[i]);
        }
        kv->put(key, df);
        delete vals;
        //delete df;
    }

    /** Idea: have put take in non-pointers

    /**
     * Contructs a DataFrame of the given schema from the given FileReader and puts it in the KVStore at the given Key
     */
    static void *fromVisitor(Key *key, KVStore *kv, char *schema, Writer *w) {
        cout << "in fromVisitor" << endl;
        Schema *s = new Schema(schema);
        DataFrame *df = new DataFrame(*s);
        while (!w->done()) {
            Row *r = new Row(s);
            w->visit(*r);
            df->add_row(*r);
            delete r;
        }
        delete s;
        cout << "done visiting" << endl;
        kv->put(key, df);
        //delete df;
    }

    /**
     * Contructs a DataFrame from the size_t and associates the given Key with the DataFrame in the given KVStore
     */
    static void *fromScalarInt(Key *key, KVStore *kv, size_t scalar) {
//        cout << "Creating df " << endl;
        Schema *s = new Schema("I");
        DataFrame *df = new DataFrame(*s);
        delete s;
//        cout << "pushing back" << endl;
        df->columns[0]->push_back((int) scalar);
        if (df->get_num_rows() < df->columns[0]->size()) {
            df->schema->nrow = df->columns[0]->size();
        }
//        cout << "putting in kv store: " << key->name->c_str()  << "size of df" << df->get_num_rows() << endl;
        kv->put(key, df);
//        cout << "done in fromScalarInt" << endl;
        //delete df;
    }


    /** Performs a step of the linus calculation. It operates over the three
     *  datafrrames (projects, users, commits), the sets of tagged users and
     *  projects, and the users added in the previous round. */
    void step(int stage) {
        cout << "\n\n\nStage " << stage << endl;
        DataFrame *chunkSoFar = new DataFrame(*new Schema("I"));

        /** in this section we chunk up newusers and send them to the nodes **/
        if (idx_ == 0) {
            StrBuff *s = new StrBuff();
            s->c("users-");
            s->c(stage);
            s->c("-0");
            String* t = s->get();
            Key *uK = new Key(t);
            delete s;
            DataFrame *newUsers = kv->get(*uK);
            cout << "newusers size: " << newUsers->get_num_rows() << endl;

            //number of chunks
            int num_chunks = 1 + ((newUsers->get_num_rows() - 1) / arg.rows_per_chunk);

            cout << "sending num chunks" << endl;
            //sending out the number of chunks each will receive
            for (size_t k = 1; k < arg.num_nodes; k++) {
                int num_received = 0;
                int selectedNode = 0;
                for (size_t w = 0; w < num_chunks; w++) {
                    if (selectedNode == k) {
                        num_received++;
                    }
                    selectedNode++;
                    if (selectedNode > arg.num_nodes - 1) {
                        selectedNode = 0;
                    }
                }
                DataFrame *node_calc = new DataFrame(*new Schema("I"));
                node_calc->columns[0]->push_back(num_received);
                Status *chunkMsg = new Status(0, k, node_calc);
                cout << "Server sending chunk" << endl;
                this->net.send_m(chunkMsg);
            }

            cout << "sent num chunks" << endl;

            /** node 0 send out newUsers to nodes in chunks **/
            // Split into chunks and send iteratively to nodes
            int selectedNode = 0;

            for (size_t j = 0; j < num_chunks; j++) {
                DataFrame *cur_chunk = newUsers->chunk(j);
                // if server's turn, keep chunks of DataFrame.
                if (selectedNode == 0) {
                    // append chunks as received
                    chunkSoFar->append_chunk(cur_chunk);
                } else {
                    // Sending to Clients
                    Status *chunkMsg = new Status(0, selectedNode, cur_chunk);
                    this->net.send_m(chunkMsg);
                }
                // increment selected node circularly between nodes
                selectedNode = ++selectedNode == arg.num_nodes ? selectedNode = 0 : selectedNode++;
            }

            cout << "sent chunks" << endl;
        } else {
            //Calculating the number of chunks and figuring out how many go to this node
            //recieve a status
            cout << "client waiting to recieve num chunks" << endl;
            Status *ipd = dynamic_cast<Status *>(this->net.recv_m());
            cout << "client recieved num chunks expected" << endl;
            int num_received = *ipd->msg_->columns[0]->as_int()->get(0);

            //recieving how many chunks theyre getting
            for (size_t i = 0; i < num_received; i++) {
                cout << "client waiting to receive chunk" << endl;
                Status *ipd = dynamic_cast<Status *>(this->net.recv_m());
                cout << "client received" << endl;

                chunkSoFar->append_chunk(ipd->msg_);
            }
        }

        /** all nodes  **/
        Set delta(users);
        cout << "set delta" << endl;
        SetUpdater *upd = new SetUpdater(delta);
        cout << "mapping" << endl;
        chunkSoFar->map(upd); // all of the new users are copied to delta.
        cout << "mapped chunksofar" << endl;
        delete upd;
        delete newUsers;

        ProjectsTagger *ptagger = new ProjectsTagger(delta, *pSet, projects);
        cout << "ptagger" << endl;
        commits->map(ptagger); // marking all projects touched by delta
        cout << "mapped commits" << endl;

        /** nodes send back commits, server merges projects **/
        this->kv = merge(ptagger->newProjects, "projects-", stage);

        cout << "first merge done" << endl;
        pSet->union_(ptagger->newProjects);

        /** server **/
        cout << "union done" << endl;
        UsersTagger *utagger = new UsersTagger(ptagger->newProjects, *uSet, users);
        delete ptagger;

        cout << "utagger done" << endl;
        commits->map(utagger);
        cout << "second merge" << endl;
        /** nodes send users and server merges **/
        this->kv = merge(utagger->newUsers, "users-", stage + 1);
        cout << "last union" << endl;
        uSet->union_(utagger->newUsers);

        delete utagger;
        cout << "    after stage " << stage << ":" << endl;
        cout << "        tagged projects: " << pSet->num_true() << endl;
        cout << "        tagged users: " << uSet->num_true() << endl;
        cout << "\n\n\nStage " << stage << endl;
    }



    /** Gather updates to the given set from all the nodes in the systems.
     * The union of those updates is then published as dataframe.  The key
     * used for the otuput is of the form "name-stage-0" where name is either
     * 'users' or 'projects', stage is the degree of separation being
     * computed.
     */
    KVStore *merge(Set &set, char const *name, int stage) {
//        cout << "in merge" << endl;
        if (this_node() == 0) {
//            cout << "found node 0" << endl;
            for (size_t i = 1; i < arg.num_nodes; ++i) {
                StrBuff *s = new StrBuff();
                s->c(name);
                s->c(stage);
                s->c("-");
                s->c(i);
                Key *nK = new Key(s->get());
                DataFrame *delta = kv->get(*nK);
                cout << "    received delta of " << delta->get_num_rows() << endl;
                cout << " elements from node " << i << endl;
                SetUpdater *upd = new SetUpdater(set);
                delta->map(upd);
                delete delta;
            }
            cout << "    storing " << set.size() << " merged elements" << endl;
            SetWriter *writer = new SetWriter(set);
//            cout << "making key" << endl;
            StrBuff *h = new StrBuff();
            h->c(name);
            h->c(stage);
            h->c("-0");
            String* str = h->get();
            Key *k = new Key(str);
            delete h;
//            cout << "k name ------- " << k->name->c_str() << endl;
            fromVisitor(k, kv, "I", writer);
//            cout << "calling fromVisitor" << endl;

//            cout << "\n\n\n" << endl;
//            for (size_t m = 0; m < kv->size; m++) {
//                cout << "KV Store Key " << m << ": " << kv->keys[m]->c_str() << endl;
//            }
//            cout << "\n\n\n" << endl;

            //DataFrame::fromVisitor(k, kv, "I", writer);
//            cout << "FROM VISITOR FINISHED" << endl;
//
//            cout << "\n\n\n" << endl;
//            for (size_t m = 0; m < kv->size; m++) {
//                cout << "KV Store Key " << m << ": " << &kv->keys[m]->name->cstr_ << endl;
//            }
//            cout << "\n\n\n" << endl;
            //return kv;

            //           cout << "kv->keys[1] NULL?" << (kv->keys[1]  == nullptr) << endl;
//            if (kv->keys[1]  != nullptr) {
//                cout << "kv->keys[1] NULL?" << kv->keys[1]->name->c_str() << endl;
//            }
        } else {
            cout << "    sending " << set.size() << " elements to master node" << endl;
            SetWriter *writer = new SetWriter(set);
            Key *k = new Key(StrBuff(name).c(stage).c("-").c(idx_).get());
            fromVisitor(k, kv, "I", writer);
            //DataFrame::fromVisitor(k, kv, "I", writer);
            Key mK(StrBuff(name).c(stage).c("-0").get());
            DataFrame *merged = dynamic_cast<DataFrame *>(kv->get(mK));
            cout << "    receiving " << merged->get_num_rows() << " merged elements" << endl;
            SetUpdater *upd = new SetUpdater(set);
            merged->map(upd);
            delete merged;
            //return nullptr;

        }
    }
}; // Linus




//
// Created by Kate Rupar on 4/9/20.
#pragma once

#include "application.h"
#include "../dataframe.h"
#include "../key.h"
#include "../reader.h"
#include "../SImap.h"
#include "../writer.h"
#include "network.h"
#include <iostream>
#include "../args.h"

using namespace std;

/*************************************************************************
 * This computes the collaborators of Linus Torvalds.
 * is the linus example using the adapter.  And slightly revised
 *   algorithm that only ever trades the deltas.
 **************************************************************************/
class Linus : public Application {
public:
    int DEGREES = 4;  // How many degrees of separation form linus?
    int LINUS = 4967;   // The uid of Linus (offset in the user df)
    bool subset = true;

    const char *PROJ = (subset ? "datasets/projects_subset.ltgt" : "datasets/projects.ltgt");
    const char *USER = (subset ? "datasets/users_subset.ltgt" : "datasets/users.ltgt");
    const char *COMM = (subset ? "datasets/commits_subset.ltgt" : "datasets/commits.ltgt");

    DataFrame *projects; //  pid x project name
    DataFrame *users;  // uid x user name
    DataFrame *commits;  // pid x uid x uid
    Set *uSet; // Linus' collaborators
    Set *pSet; // projects of collaborators

    Linus(size_t idx, NetworkIP &net) : Application(idx, net) {}

    /** Compute DEGREES of Linus.  */
    void run_() override {
        readInput();
        for (size_t i = 0; i < DEGREES; i++) step(i);
    }

    /** Node 0 reads three files, cointainng projects, users and commits, and
     *  creates thre dataframes. All other nodes wait and load the three
     *  dataframes. Once we know the size of users and projects, we create
     *  sets of each (uSet and pSet). We also output a data frame with a the
     *  'tagged' users. At this point the dataframe consists of only
     *  Linus. **/
    void readInput() {
        Key *pK = new Key("projs");
        Key *uK = new Key("usrs");
        Key *cK = new Key("comts");
        if (this_node() == 0) {
            // We never put these dfs in the store???
            cout << "Reading..." << endl;
            commits = DataFrame::fromFile(COMM, cK, kv);
            cout << "    " << commits->get_num_rows() << " commits" << endl;
            projects = DataFrame::fromFile(PROJ, pK, kv);
            cout << "    " << projects->get_num_rows() << " projects" << endl;
            users = DataFrame::fromFile(USER, uK, kv);
            cout << "    " << users->get_num_rows() << " users" << endl;
            // This dataframe contains the id of Linus.
            //delete
            Key *key = new Key("users-0-0");
            DataFrame::fromScalarInt(key, kv, LINUS);
        } else {
            cout << "Reading..." << endl;
            commits = DataFrame::fromFile(COMM, cK, kv);
            cout << "    " << commits->get_num_rows() << " commits" << endl;
            projects = DataFrame::fromFile(PROJ, pK, kv);
            cout << "    " << projects->get_num_rows() << " projects" << endl;
            users = DataFrame::fromFile(USER, uK, kv);
            cout << "    " << users->get_num_rows() << " users" << endl;
        }
        cout << "making sets" << endl;
        uSet = new Set(users);
        cout << "made a set" << endl;
        pSet = new Set(projects);
        cout << "done making sets" << endl;
    }

    /**
     * Ideas:
     * break up newusers send in chunks to every node
     * node do projects and then merge
     *
     *
     */



    /** Performs a step of the linus calculation. It operates over the three
     *  datafrrames (projects, users, commits), the sets of tagged users and
     *  projects, and the users added in the previous round. */
    void step(int stage) {
        cout << "\n\n\nStage " << stage << endl;
        DataFrame *chunkSoFar = new DataFrame(*new Schema("I"));

        /** in this section we chunk up newusers and send them to the nodes **/
        if (idx_ == 0) {
            StrBuff *s = new StrBuff();
            s->c("users-");
            s->c(stage);
            s->c("-0");
            Key *uK = new Key(s->get());
            DataFrame *newUsers = kv->get(*uK);
            cout << "newusers size: " << newUsers->get_num_rows() << endl;

            //number of chunks
            int num_chunks = 1 + ((newUsers->get_num_rows() - 1) / arg.rows_per_chunk);

            cout << "sending num chunks" << endl;
            //sending out the number of chunks each will receive
            for (size_t k = 1; k < arg.num_nodes; k++) {
                int num_received = 0;
                int selectedNode = 0;
                for (size_t w = 0; w < num_chunks; w++) {
                    if (selectedNode == k) {
                        num_received++;
                    }
                    selectedNode++;
                    if (selectedNode > arg.num_nodes - 1) {
                        selectedNode = 0;
                    }
                }
                DataFrame *node_calc = new DataFrame(*new Schema("I"));
                node_calc->columns[0]->push_back(num_received);
                Status *chunkMsg = new Status(0, k, node_calc);
                cout << "Server sending chunk" << endl;
                this->net.send_m(chunkMsg);
            }

            cout << "sent num chunks" << endl;

            /** node 0 send out newUsers to nodes in chunks **/
            // Split into chunks and send iteratively to nodes
            int selectedNode = 0;

            for (size_t j = 0; j < num_chunks; j++) {
                DataFrame *cur_chunk = newUsers->chunk(j);
                // if server's turn, keep chunks of DataFrame.
                if (selectedNode == 0) {
                    // append chunks as received
                    chunkSoFar->append_chunk(cur_chunk);
                } else {
                    // Sending to Clients
                    Status *chunkMsg = new Status(0, selectedNode, cur_chunk);
                    this->net.send_m(chunkMsg);
                }
                // increment selected node circularly between nodes
                selectedNode = ++selectedNode == arg.num_nodes ? selectedNode = 0 : selectedNode++;
            }

            cout << "sent chunks" << endl;
        } else {
            //Calculating the number of chunks and figuring out how many go to this node
            //recieve a status
            cout << "client waiting to recieve num chunks" << endl;
            Status *ipd = dynamic_cast<Status *>(this->net.recv_m());
            cout << "client recieved num chunks expected" << endl;
            int num_received = *ipd->msg_->columns[0]->as_int()->get(0);

            //recieving how many chunks theyre getting
            for (size_t i = 0; i < num_received; i++) {
                cout << "client waiting to receive chunk" << endl;
                Status *ipd = dynamic_cast<Status *>(this->net.recv_m());
                cout << "client received" << endl;

                chunkSoFar->append_chunk(ipd->msg_);
            }
        }


        /** all nodes  **/
        Set delta(users);
        cout << "set delta" << endl;
        SetUpdater *upd = new SetUpdater(delta);
        cout << "mapping" << endl;
        chunkSoFar->map(upd); // all of the new users are copied to delta.
        cout << "mapped chunksofar" << endl;
        //delete newUsers;
        ProjectsTagger *ptagger = new ProjectsTagger(delta, *pSet, projects);
        cout << "ptagger" << endl;
        commits->map(ptagger); // marking all projects touched by delta
        cout << "mapped commits" << endl;

        /** nodes send back commits, server merges projects **/
        this->kv = merge(ptagger->newProjects, "projects-", stage);

        cout << "first merge done" << endl;
        pSet->union_(ptagger->newProjects);

        /** server **/
        cout << "union done" << endl;
        UsersTagger *utagger = new UsersTagger(ptagger->newProjects, *uSet, users);

        cout << "utagger done" << endl;
        commits->map(utagger);
        cout << "second merge" << endl;
        /** nodes send users and server merges **/
        this->kv = merge(utagger->newUsers, "users-", stage + 1);
        cout << "last union" << endl;
        uSet->union_(utagger->newUsers);

        cout << "    after stage " << stage << ":" << endl;
        cout << "        tagged projects: " << pSet->size() << endl;
        cout << "        tagged users: " << uSet->size() << endl;
    }

    /** Gather updates to the given set from all the nodes in the systems.
     * The union of those updates is then published as dataframe.  The key
     * used for the otuput is of the form "name-stage-0" where name is either
     * 'users' or 'projects', stage is the degree of separation being
     * computed.
     */
    KVStore *merge(Set &set, char const *name, int stage) {
//        cout << "in merge" << endl;
        if (this_node() == 0) {
//            cout << "found node 0" << endl;
            cout << "in merge server" << endl;
            for (size_t i = 1; i < arg.num_nodes; ++i) {
//                StrBuff *s = new StrBuff();
//                s->c(name);
//                s->c(stage);
//                s->c("-");
//                s->c(i);
//                Key *nK = new Key(s->get());
                //getting a dataframe delta from every node
                //DataFrame *delta = kv->get(*nK);

                cout << "recieving..." << endl;
                Status *msg = dynamic_cast<Status *>(this->net.recv_m());
                cout << "recieved." << endl;
                DataFrame *delta = msg->msg_;

                cout << "    received delta of " << delta->get_num_rows() << endl;
                cout << " elements from node " << i << endl;
                SetUpdater *upd = new SetUpdater(set);
                delta->map(upd);
                //delete delta;
            }
            cout << "    storing " << set.size() << " merged elements" << endl;
            SetWriter *writer = new SetWriter(set);
//            cout << "making key" << endl;
            StrBuff *h = new StrBuff();
            h->c(name);
            h->c(stage);
            h->c("-0");
            Key *k = new Key(h->get());
//            cout << "k name ------- " << k->name->c_str() << endl;
            //delete DataFrame::fromVisitor(&k, &kv, "I", writer);
//            cout << "calling fromVisitor" << endl;

//            cout << "\n\n\n" << endl;
//            for (size_t m = 0; m < kv->size; m++) {
//                cout << "KV Store Key " << m << ": " << kv->keys[m]->c_str() << endl;
//            }
//            cout << "\n\n\n" << endl;

            DataFrame::fromVisitor(k, kv, "I", writer);
//            cout << "FROM VISITOR FINISHED" << endl;
//
//            cout << "\n\n\n" << endl;
//            for (size_t m = 0; m < kv->size; m++) {
//                cout << "KV Store Key " << m << ": " << &kv->keys[m]->name->cstr_ << endl;
//            }
//            cout << "\n\n\n" << endl;
            return kv;

            //           cout << "kv->keys[1] NULL?" << (kv->keys[1]  == nullptr) << endl;
//            if (kv->keys[1]  != nullptr) {
//                cout << "kv->keys[1] NULL?" << kv->keys[1]->name->c_str() << endl;
//            }
        } else {
            cout << "    sending " << set.size() << " elements to master node" << endl;
            SetWriter *writer = new SetWriter(set);
            Key k(StrBuff(name).c(stage).c("-").c(idx_).get());
//            delete DataFrame::fromVisitor(&k, &kv, "I", writer);
            DataFrame *toSend = DataFrame::fromVisitor(k, kv, "I", writer);
            Status *nodeToServer = new Status(idx_, 0, toSend);
            this->net.send_m(nodeToServer);

//            Key mK(StrBuff(name).c(stage).c("-0").get());
//            DataFrame *merged = dynamic_cast<DataFrame *>(kv->get(mK));
//            cout << "    receiving " << merged->get_num_rows() << " merged elements" << endl;
//            SetUpdater *upd = new SetUpdater(set);
//            merged->map(upd);
            return nullptr;
            //delete merged;
        }
    }
}; // Linus

