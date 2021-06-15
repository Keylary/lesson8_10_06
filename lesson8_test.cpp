#include <iostream>
#include "students.pb.h"
#include <fstream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <gtest\gtest.h>
#include <sstream>


using namespace std;

class FullName {
protected:
    std::string m_last_name;
    std::string m_first_name;
    std::string m_patronymic = "";

public:

    FullName() {}
    FullName(std::string last_name, std::string first_name, std::string patronymic) : m_last_name(last_name), m_first_name(first_name), m_patronymic(patronymic) {}

    FullName(const serialization::FullName& fn) {
        m_last_name = fn.last_name();
        m_first_name = fn.first_name();
        m_patronymic = fn.patronymic();
    }
    void ConvertToSerializationType(serialization::FullName* fn) {
        fn->set_last_name(m_last_name);
        fn->set_first_name(m_first_name);
        fn->set_patronymic(m_patronymic);
    }

    bool has_patronymic() { return (m_patronymic.size() != 0); };

    friend bool operator==(const FullName& fn1, const FullName& fn2) {
        return fn1.m_last_name == fn2.m_last_name && fn1.m_first_name == fn2.m_first_name && fn1.m_patronymic == fn2.m_patronymic;
    }
    friend std::ostream& operator<<(std::ostream& out, const FullName& fn) {
        out << fn.m_last_name << " " << fn.m_first_name << " " << fn.m_patronymic;
        return out;
    }

    // методы для тестирования
    std::string last_name() const { return m_last_name; }
    std::string first_name() const { return m_first_name; }
    std::string patronymic() const { return m_patronymic; }

    std::string print() const {
        std::ostringstream os;
        os << m_last_name << " " << m_first_name << " " << m_patronymic;
        return os.str();
    }
    
    void set_last_name(std::string last_name) { m_last_name = last_name; }
    void set_first_name(std::string first_name) { m_first_name = first_name; }
    void set_patronymic(std::string patronymic) { m_patronymic = patronymic; }


};



// Task 2. ТЕСТЫ


class TestFullName : public testing::Test {
protected:
    void SetUp() override {
        name = new FullName("Darnizkaya", "Ilana", "Grigorievna");
    }
    void TearDown() override {
        delete name;
    }
    FullName* name;
};

TEST_F(TestFullName, get_methods) {
    ASSERT_EQ(name->last_name(), "Darnizkaya");
    ASSERT_EQ(name->first_name(), "Ilana");
    ASSERT_EQ(name->patronymic(), "Grigorievna");
}

TEST_F(TestFullName, set_methods) {
    name->set_last_name("XXX");
    ASSERT_EQ(name->last_name(), "XXX");
    name->set_first_name("YYY");
    ASSERT_EQ(name->first_name(), "YYY");
    name->set_patronymic("ZZZ");
    ASSERT_EQ(name->patronymic(), "ZZZ");
}

TEST_F(TestFullName, print) {
    ASSERT_STREQ(name->print().c_str(), "Darnizkaya Ilana Grigorievna") << std::endl;
}

class Student : public FullName {
protected:
    FullName m_fullName;
    std::vector<int32_t> m_scores;
    int32_t m_averageScore;
public:

    Student(const FullName& fullName, std::vector<int32_t> scores, int32_t averageScore)
        : m_fullName(fullName), m_scores(scores), m_averageScore(averageScore) { }
    Student(const serialization::Student& st) {
        m_averageScore = st.averagescore();
        for (size_t i = 0; i < st.score_size(); ++i) {
            m_scores.push_back(st.score(i));
        }
        m_fullName = FullName(st.m_fullname());
    }

    void ConvertToSerializationType(serialization::Student* st) {
        std::for_each(m_scores.begin(), m_scores.end(), [&st](int value) { st->add_score(value); });
        st->set_averagescore(m_averageScore);
        serialization::FullName* fn = st->mutable_m_fullname();
        m_fullName.ConvertToSerializationType(fn);
    }

    friend std::ostream& operator<<(std::ostream& out, const Student& st) {
        out << st.m_fullName << " ";
        copy(st.m_scores.begin(), st.m_scores.end(), std::ostream_iterator<int>(out, " "));
        return out;
    }

    const FullName& fullName() const { return m_fullName; }
    const std::vector<int>& scores() const { return m_scores; }

};

class IRepository {
    virtual void Open() = 0;
    virtual void Save() = 0;
};

class IMethods {
    virtual double GetAverageScore(const FullName& name) = 0;
    virtual std::string GetAllInfo(const FullName& name) = 0;
    virtual std::string GetAllInfo() = 0;
};

class StudentsGroup : public IRepository, public IMethods
{
private:
    std::vector<Student> m_students;
    serialization::StudentsGroup ConvertToSerializationType(std::vector<Student> students) {
        serialization::StudentsGroup sg;
        for (auto student : students) {
            serialization::Student* st = sg.add_students();
            student.ConvertToSerializationType(st);
        }
        return sg;
    }
    std::vector<Student> ConvertFromSerializationType(serialization::StudentsGroup students) {
        std::vector<Student> sg;
        for (size_t i = 0; i < students.students_size(); i++) {
            serialization::Student st = students.students(i);
            sg.push_back(Student(students.students(i)));
        }
        return sg;
    }
public:
    StudentsGroup() { }
    StudentsGroup(std::vector<Student> students) : m_students(students) { }

    void Open() override {
        serialization::StudentsGroup s;
        std::ifstream in("students.bin", std::ios_base::binary);

        if (!s.ParseFromIstream(&in)) {
            throw new std::exception("error with serialization");
        }

        m_students = ConvertFromSerializationType(s);
    }
    void Save() override {

        serialization::StudentsGroup s = ConvertToSerializationType(m_students);
        std::ofstream out("students.bin", std::ios_base::binary);
        s.SerializeToOstream(&out);
    }

    friend std::ostream& operator<<(std::ostream& out, const StudentsGroup& sg) {
        copy(sg.m_students.begin(), sg.m_students.end(), std::ostream_iterator<Student>(out, "\n"));
        return out;
    }

    double GetAverageScore(const FullName& name) override {
        auto finded = find_if(m_students.begin(), m_students.end(), [&](const Student& st) {return name == st.fullName(); });
        if (finded != m_students.end()) {
            auto scores = finded->scores();
            return std::accumulate(scores.begin(), scores.end(), 0) / scores.size();
        }
        return 0;
    }
    std::string GetAllInfo(const FullName& name) override {
        auto finded = find_if(m_students.begin(), m_students.end(), [&](const Student& st) {return name == st.fullName(); });
        if (finded != m_students.end()) {
            std::ostringstream ss;
            ss << *finded;
            return ss.str();
        }
        return std::string();
    }
    std::string GetAllInfo() override {
        std::ostringstream ss;
        ss << *this;
        return ss.str();
    }


};




// Task 2. Тесты


class TestStudGroup : public testing::Test {
protected:
    
    void SetUp() override {
        group = new StudentsGroup(std::vector<Student> {
            Student(FullName("Ivanova", "Elena", "Sergeevna"), std::vector<int32_t>{ 1, 2, 3, 2, 2 }, 2)});

    }
    void TearDown() override {
        delete group;
    }
    StudentsGroup* group;

};

// Специальное несовпадение


TEST_F(TestStudGroup, GetAllInfo) {
    ASSERT_STREQ(group->GetAllInfo().c_str(), "XXX YYY ZZZ") << std::endl;
}




int main()
{

    StudentsGroup group1(
        std::vector<Student> {
        Student(FullName("Ivanova", "Ioanna", ""), std::vector<int32_t>{ 1, 2, 3, 2, 2 }, 2),
            Student(FullName("Petrov", "Petr", "Petrovich"), std::vector<int32_t>{ 5, 3, 1, 4, 2 }, 3),
            Student(FullName("Sidorov", "Sidor", "Sidorovich"), std::vector<int32_t>{ 5, 5, 4, 3, 3 }, 4)
    });

    group1.Save();
    StudentsGroup group2;
    group2.Open();

    std::cout << group2.GetAverageScore(FullName("Rogova", "Viktoriia", "Matveevna")) << std::endl;
    std::cout << group2.GetAllInfo(FullName("Rogova", "Viktoriia", "Matveevna")) << std::endl;
    std::cout << group2.GetAllInfo() << std::endl;

  
    
    // Task 2. ТЕСТЫ

    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}

