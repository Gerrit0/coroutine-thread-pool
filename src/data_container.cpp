#include <memory>
#include <type_traits>
#include <vector>
#include <variant>

#include <boost/iostreams/device/mapped_file.hpp>

#include "data_container.h"
#include "logger.h"

static_assert(std::is_trivial_v<Datum>, "Must be trivial to safely reinterpret cast from data in file");

constexpr size_t max_in_memory_data_size = 1024;

template <class... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};

class DataContainerImpl
{
    std::string m_file_name;
    std::variant<boost::iostreams::mapped_file, std::vector<Datum>> m_data;
    Datum *m_data_ptr;
    size_t m_capacity;
    size_t m_size;

public:
    DataContainerImpl(size_t id) : m_file_name(std::to_string(id) + ".bin"),
                                   m_data(std::vector<Datum>(1)),
                                   m_data_ptr(std::get<std::vector<Datum>>(m_data).data()),
                                   m_capacity(1),
                                   m_size(0)
    {
    }

    ~DataContainerImpl()
    {
        std::visit(overloaded{[](std::vector<Datum> &) {},
                              [&](boost::iostreams::mapped_file &file)
                              {
                                  file.close();
                                  if (remove(m_file_name.c_str()) != 0)
                                  {
                                      Logger::printf("Failed to remove temporary file %s - %s", m_file_name.c_str(), strerror(errno));
                                  }
                              }},
                   m_data);
    }

    Datum &at(size_t index)
    {
        assert(index < m_size);
        return m_data_ptr[index];
    }

    void push_back(Datum datum)
    {
        if (m_size + 1 > m_capacity)
        {
            if (m_capacity <= max_in_memory_data_size && m_capacity * 2 > max_in_memory_data_size)
            {
                // Switch from vector to file based storage
                m_capacity *= 2;

                auto max_size_bytes = sizeof(Datum) * m_capacity;
                auto data = std::move(std::get<std::vector<Datum>>(m_data));
                boost::iostreams::mapped_file file;
                boost::iostreams::mapped_file_params params;
                params.path = m_file_name;
                params.new_file_size = static_cast<boost::iostreams::stream_offset>(max_size_bytes);
                params.flags = boost::iostreams::mapped_file::readwrite;
                file.open(params);

                if (file.size() < max_size_bytes)
                {
                    // This should only happen someone kills the program and we end up re-using a file.
                    Logger::printf("Resizing to %zu bytes\n", max_size_bytes);
                    file.resize(static_cast<boost::iostreams::stream_offset>(max_size_bytes));
                }

                m_data_ptr = reinterpret_cast<Datum *>(file.data());
                memcpy(m_data_ptr, data.data(), sizeof(Datum) * data.size());
                m_data = std::move(file);
            }
            else
            {
                m_capacity *= 2;

                std::visit(overloaded{[&](std::vector<Datum> &vec)
                                      {
                                          vec.resize(m_capacity);
                                          m_data_ptr = vec.data();
                                      },
                                      [&](boost::iostreams::mapped_file &file)
                                      {
                                          file.resize(static_cast<boost::iostreams::stream_offset>(m_capacity * sizeof(Datum)));
                                          m_data_ptr = reinterpret_cast<Datum *>(file.data());
                                      }},
                           m_data);
            }
        }

        m_data_ptr[m_size] = datum;
        ++m_size;
    }

    size_t size() const
    {
        return m_size;
    }
};

DataContainer::DataContainer(size_t id) : m_pimpl(new DataContainerImpl(id))
{
}

DataContainer::~DataContainer()
{
    delete m_pimpl;
}

Datum &DataContainer::at(size_t index)
{
    return m_pimpl->at(index);
}

void DataContainer::push_back(Datum datum)
{
    m_pimpl->push_back(std::move(datum));
}

size_t DataContainer::size() const
{
    return m_pimpl->size();
}
