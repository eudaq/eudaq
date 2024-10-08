// Copyright 2013-2023 Daniel Parker
// Distributed under Boost license

#include <jsoncons/json.hpp>
#include <jsoncons_ext/csv/csv.hpp>
#include <fstream>
#include <iomanip>
#include <cassert>
#include "sample_types.hpp"

using namespace jsoncons;

void encode_n_objects()
{
    const std::string input = R"(
[
    {
        "customer_name": "John Roe",
        "has_coupon": true,
        "phone_number": "0272561313",
        "zip_code": "01001",
        "sales_tax_rate": 0.05,
        "total_amount": 431.65
    },
    {
        "customer_name": "Jane Doe",
        "has_coupon": false,
        "phone_number": "416-272-2561",
        "zip_code": "55416",
        "sales_tax_rate": 0.15,
        "total_amount": 480.7
    },
    {
        "customer_name": "Joe Bloggs",
        "has_coupon": false,
        "phone_number": "4162722561",
        "zip_code": "55416",
        "sales_tax_rate": 0.15,
        "total_amount": 300.7
    },
    {
        "customer_name": "John Smith",
        "has_coupon": false,
        "phone_number": null,
        "zip_code": "22313-1450",
        "sales_tax_rate": 0.15,
        "total_amount": 300.7
    }
]
    )";

    ojson j = ojson::parse(input);

    std::string output;
    csv::csv_options ioptions;
    ioptions.quote_style(csv::quote_style_kind::nonnumeric);
    csv::encode_csv(j, output, ioptions);
    std::cout << output << "\n\n";

    csv::csv_options ooptions;
    ooptions.assume_header(true);
    ojson other = csv::decode_csv<ojson>(output, ooptions);
    assert(other == j);
}

void encode_n_rows()
{
    const std::string input = R"(
[
    ["customer_name","has_coupon","phone_number","zip_code","sales_tax_rate","total_amount"],
    ["John Roe",true,"0272561313","01001",0.05,431.65],
    ["Jane Doe",false,"416-272-2561","55416",0.15,480.7],
    ["Joe Bloggs",false,"4162722561","55416",0.15,300.7],
    ["John Smith",false,null,"22313-1450",0.15,300.7]
]
    )";

    json j = json::parse(input);

    std::string output;
    csv::csv_options ioptions;
    ioptions.quote_style(csv::quote_style_kind::nonnumeric);
    csv::encode_csv(j, output, ioptions);
    std::cout << output << "\n\n";

    json other = csv::decode_csv<json>(output);
    assert(other == j);
}

void encode_m_columns()
{
    const std::string input = R"(
{
    "customer_name": ["John Roe","Jane Doe","Joe Bloggs","John Smith"],
    "has_coupon": [true,false,false,false],
    "phone_number": ["0272561313","416-272-2561","4162722561",null],
    "zip_code": ["01001","55416","55416","22313-1450"],
    "sales_tax_rate": [0.05,0.15,0.15,0.15],
    "total_amount": [431.65,480.7,300.7,300.7]
}
    )";

    ojson j = ojson::parse(input);

    std::string output;
    csv::csv_options ioptions;
    ioptions.quote_style(csv::quote_style_kind::nonnumeric);
    csv::encode_csv(j, output, ioptions);
    std::cout << output << "\n\n";

    csv::csv_options ooptions;
    ooptions.assume_header(true)
            .mapping_kind(csv::csv_mapping_kind::m_columns);
    ojson other = csv::decode_csv<ojson>(output, ooptions);
    assert(other == j);
}

void csv_source_to_json_value()
{
    const std::string s = R"(Date,1Y,2Y,3Y,5Y
2017-01-09,0.0062,0.0075,0.0083,0.011
2017-01-08,0.0063,0.0076,0.0084,0.0112
2017-01-08,0.0063,0.0076,0.0084,0.0112
)";

    csv::csv_options options;
    options.assume_header(true)
           .column_types("string,float,float,float,float");

    // csv_mapping_kind::n_objects
    options.mapping_kind(csv::csv_mapping_kind::n_objects);
    ojson j1 = csv::decode_csv<ojson>(s,options);
    std::cout << "\n(1)\n"<< pretty_print(j1) << "\n";

    // csv_mapping_kind::n_rows
    options.mapping_kind(csv::csv_mapping_kind::n_rows);
    ojson j2 = csv::decode_csv<ojson>(s,options);
    std::cout << "\n(2)\n"<< pretty_print(j2) << "\n";

    // csv_mapping_kind::m_columns
    options.mapping_kind(csv::csv_mapping_kind::m_columns);
    ojson j3 = csv::decode_csv<ojson>(s,options);
    std::cout << "\n(3)\n" << pretty_print(j3) << "\n";
}

void csv_source_to_cpp_object()
{
    const std::string input = R"(Date,1Y,2Y,3Y,5Y
2017-01-09,0.0062,0.0075,0.0083,0.011
2017-01-08,0.0063,0.0076,0.0084,0.0112
2017-01-08,0.0063,0.0076,0.0084,0.0112
)";

    csv::csv_options ioptions;
    ioptions.header_lines(1)
            .mapping_kind(csv::csv_mapping_kind::n_rows);

    using table_type = std::vector<std::tuple<std::string,double,double,double,double>>;

    table_type table = csv::decode_csv<table_type>(input,ioptions);

    std::cout << "(1)\n";
    for (const auto& row : table)
    {
        std::cout << std::get<0>(row) << "," 
                  << std::get<1>(row) << "," 
                  << std::get<2>(row) << "," 
                  << std::get<3>(row) << "," 
                  << std::get<4>(row) << "\n";
    }
    std::cout << "\n";

    std::string output;

    csv::csv_options ooptions;
    ooptions.column_names("Date,1Y,2Y,3Y,5Y");
    csv::encode_csv<table_type>(table, output, ooptions);

    std::cout << "(2)\n";
    std::cout << output << "\n";
}

void csv_decode_without_type_inference()
{
    std::string s = R"(employee-no,employee-name,dept,salary
00000001,"Smith,Matthew",sales,150000.00
00000002,"Brown,Sarah",sales,89000.00
)";

    csv::csv_options options;
    options.assume_header(true)
           .infer_types(false);
    ojson j = csv::decode_csv<ojson>(s,options);

    std::cout << pretty_print(j) << std::endl;
}

void read_write_csv_tasks()
{
    std::ifstream is("./input/tasks.csv");

    json_decoder<ojson> decoder;
    csv::csv_options options;
    options.assume_header(true)
           .trim(true)
           .ignore_empty_values(true) 
           .column_types("integer,string,string,string");
    csv::csv_stream_reader reader(is,decoder,options);
    reader.read();
    ojson tasks = decoder.get_result();

    std::cout << "(1)\n";
    std::cout << pretty_print(tasks) << "\n\n";

    std::cout << "(2)\n";
    csv::csv_stream_encoder encoder(std::cout);
    tasks.dump(encoder);
}

void serialize_array_of_arrays_to_comma_delimited()
{
    std::string in_file = "./input/countries.json";
    std::ifstream is(in_file);

    json countries;
    is >> countries;

    csv::csv_stream_encoder encoder(std::cout);
    countries.dump(encoder);
}

void encode_to_tab_delimited_file()
{
    std::string in_file = "./input/employees.json";
    std::ifstream is(in_file);

    json employees;
    is >> employees;

    csv::csv_options options;
    options.field_delimiter('\t');
    csv::csv_stream_encoder encoder(std::cout,options);

    employees.dump(encoder);
}

void serialize_books_to_csv_file()
{
    const json books = json::parse(R"(
    [
        {
            "title" : "Kafka on the Shore",
            "author" : "Haruki Murakami",
            "price" : 25.17
        },
        {
            "title" : "Women: A Novel",
            "author" : "Charles Bukowski",
            "price" : 12.00
        },
        {
            "title" : "Cutter's Way",
            "author" : "Ivan Passer"
        }
    ]
    )");

    csv::csv_stream_encoder encoder(std::cout);

    books.dump(encoder);
}

void serialize_books_to_csv_file_with_reorder()
{
    const json books = json::parse(R"(
    [
        {
            "title" : "Kafka on the Shore",
            "author" : "Haruki Murakami",
            "price" : 25.17
        },
        {
            "title" : "Women: A Novel",
            "author" : "Charles Bukowski",
            "price" : 12.00
        },
        {
            "title" : "Cutter's Way",
            "author" : "Ivan Passer"
        }
    ]
    )");

    csv::csv_options options;
    options.column_names("author,title,price");

    csv::csv_stream_encoder encoder(std::cout, options);

    books.dump(encoder);
}

void last_column_repeats()
{
    const std::string bond_yields = R"(Date,Yield
2017-01-09,0.0062,0.0075,0.0083,0.011,0.012
2017-01-08,0.0063,0.0076,0.0084,0.0112,0.013
2017-01-08,0.0063,0.0076,0.0084,0.0112,0.014
)";

    json_decoder<ojson> decoder1;
    csv::csv_options options1;
    options1.header_lines(1);
    options1.column_types("string,float*");
    std::istringstream is1(bond_yields);
    csv::csv_stream_reader reader1(is1, decoder1, options1);
    reader1.read();
    ojson val1 = decoder1.get_result();
    std::cout << "\n(1)\n" << pretty_print(val1) << "\n";

    json_decoder<ojson> decoder2;
    csv::csv_options options2;
    options2.assume_header(true);
    options2.column_types("string,[float*]");
    std::istringstream is2(bond_yields);
    csv::csv_stream_reader reader2(is2, decoder2, options2);
    reader2.read();
    ojson val2 = decoder2.get_result();
    std::cout << "\n(2)\n" << pretty_print(val2) << "\n";
}

void last_two_columns_repeat()
{
    const std::string holidays = R"(1,CAD,2,UK,3,EUR,4,US
38719,2-Jan-2006,40179,1-Jan-2010,38719,2-Jan-2006,39448,1-Jan-2008
38733,16-Jan-2006,40270,2-Apr-2010,38733,16-Jan-2006,39468,21-Jan-2008
)";

    // array of arrays
    json_decoder<ojson> decoder1;
    csv::csv_options options1;
    options1.column_types("[integer,string]*");
    std::istringstream is1(holidays);
    csv::csv_stream_reader reader1(is1, decoder1, options1);
    reader1.read();
    ojson val1 = decoder1.get_result();
    std::cout << "(1)\n" << pretty_print(val1) << "\n";

    // array of objects
    json_decoder<ojson> decoder2;
    csv::csv_options options2;
    options2.header_lines(1);
    options2.column_names("CAD,UK,EUR,US");
    options2.column_types("[integer,string]*");
    std::istringstream is2(holidays);
    csv::csv_stream_reader reader2(is2, decoder2, options2);
    reader2.read();
    ojson val2 = decoder2.get_result();
    std::cout << "(2)\n" << pretty_print(val2) << "\n";
}

void decode_csv_string()
{
    std::string s = R"(employee-no,employee-name,dept,salary
00000001,\"Smith,Matthew\",sales,150000.00
00000002,\"Brown,Sarah\",sales,89000.00
)";

    csv::csv_options options;
    options.assume_header(true)
           .column_types("string,string,string,float");
    json j = csv::decode_csv<json>(s,options);

    std::cout << pretty_print(j) << std::endl;
}

void decode_csv_stream()
{
    const std::string bond_yields = R"(Date,1Y,2Y,3Y,5Y
2017-01-09,0.0062,0.0075,0.0083,0.011
2017-01-08,0.0063,0.0076,0.0084,0.0112
2017-01-07,0.0063,0.0076,0.0084,0.0112
)";

    csv::csv_options options;
    options.assume_header(true)
           .column_types("string,float,float,float,float");

    std::istringstream is(bond_yields);

    ojson j = csv::decode_csv<ojson>(is,options);

    std::cout << pretty_print(j) << std::endl;
}

void encode_csv_file_from_books()
{
    const json books = json::parse(R"(
    [
        {
            "title" : "Kafka on the Shore",
            "author" : "Haruki Murakami",
            "price" : 25.17
        },
        {
            "title" : "Women: A Novel",
            "author" : "Charles Bukowski",
            "price" : 12.00
        },
        {
            "title" : "Cutter's Way",
            "author" : "Ivan Passer"
        }
    ]
    )");

    csv::encode_csv(books, std::cout);
}

void decode_encode_csv_tasks()
{
    std::ifstream is("./input/tasks.csv");

    csv::csv_options options;
    options.assume_header(true)
           .trim(true)
           .ignore_empty_values(true) 
           .column_types("integer,string,string,string");
    ojson tasks = csv::decode_csv<ojson>(is, options);

    std::cout << "(1)\n" << pretty_print(tasks) << "\n\n";

    std::cout << "(2)\n";
    csv::encode_csv(tasks, std::cout);
}

void csv_parser_type_inference()
{
    csv::csv_options options;
    options.assume_header(true)
           .mapping_kind(csv::csv_mapping_kind::n_objects);

    std::ifstream is1("input/sales.csv");
    ojson j1 = csv::decode_csv<ojson>(is1,options);
    std::cout << "\n(1)\n"<< pretty_print(j1) << "\n";

    options.mapping_kind(csv::csv_mapping_kind::n_rows);
    std::ifstream is2("input/sales.csv");
    ojson j2 = csv::decode_csv<ojson>(is2,options);
    std::cout << "\n(2)\n"<< pretty_print(j2) << "\n";

    options.mapping_kind(csv::csv_mapping_kind::m_columns);
    std::ifstream is3("input/sales.csv");
    ojson j3 = csv::decode_csv<ojson>(is3,options);
    std::cout << "\n(3)\n"<< pretty_print(j3) << "\n";
}
 
// Examples with subfields
 

void decode_csv_with_subfields()
{
    const std::string s = R"(calculationPeriodCenters,paymentCenters,resetCenters
NY;LON,TOR,LON
NY,LON,TOR;LON
"NY";"LON","TOR","LON"
"NY","LON","TOR";"LON"
)";
    csv::csv_options options1;
    options1.assume_header(true)
            .subfield_delimiter(';');

    json j1 = csv::decode_csv<json>(s,options1);

    json_options print_options;
    print_options.array_array_line_splits(line_split_kind::same_line)
                 .float_format(float_chars_format::fixed);

    std::cout << "(1)\n" << pretty_print(j1,print_options) << "\n\n";

    csv::csv_options options2;
    options2.mapping_kind(csv::csv_mapping_kind::n_rows)
           .subfield_delimiter(';');

    json j2 = csv::decode_csv<json>(s,options2);
    std::cout << "(2)\n" << pretty_print(j2,print_options) << "\n\n";

    csv::csv_options options3;
    options3.assume_header(true)
           .mapping_kind(csv::csv_mapping_kind::m_columns)
           .subfield_delimiter(';');

    json j3 = csv::decode_csv<json>(s,options3);
    std::cout << "(3)\n" << pretty_print(j3,print_options) << "\n\n";
}

    const std::string data = R"(index_id,observation_date,rate
EUR_LIBOR_06M,2015-10-23,0.0000214
EUR_LIBOR_06M,2015-10-26,0.0000143
EUR_LIBOR_06M,2015-10-27,0.0000001
)";

void as_a_variant_like_structure()
{
    csv::csv_options options;
    options.assume_header(true);

    // Parse the CSV data into an ojson value
    ojson j = csv::decode_csv<ojson>(data, options);

    // Pretty print
    json_options print_options;
    print_options.float_format(float_chars_format::fixed);
    std::cout << "(1)\n" << pretty_print(j, print_options) << "\n\n";

    // Iterate over the rows
    std::cout << std::fixed << std::setprecision(7);
    std::cout << "(2)\n";
    for (const auto& row : j.array_range())
    {
        // Access rated as string and rating as double
        std::cout << row["index_id"].as<std::string>() << ", " 
                  << row["observation_date"].as<std::string>() << ", " 
                  << row["rate"].as<double>() << "\n";
    }
}

void as_a_strongly_typed_cpp_structure()
{
    csv::csv_options options;
    options.assume_header(true)
           .float_format(float_chars_format::fixed);

    // Decode the CSV data into a c++ structure
    std::vector<ns::fixing> v = csv::decode_csv<std::vector<ns::fixing>>(data, options);

    // Iterate over values
    std::cout << std::fixed << std::setprecision(7);
    std::cout << "(1)\n";
    for (const auto& item : v)
    {
        std::cout << item.index_id() << ", " << item.observation_date() << ", " << item.rate() << "\n";
    }

    // Encode the c++ structure into CSV data
    std::string s;
    csv::encode_csv(v, s, options);
    std::cout << "(2)\n";
    std::cout << s << "\n";
}

void as_a_stream_of_json_events()
{
    csv::csv_options options;
    options.assume_header(true);

    csv::csv_string_cursor cursor(data, options);

    for (; !cursor.done(); cursor.next())
    {
        const auto& event = cursor.current();
        switch (event.event_type())
        {
            case staj_event_type::begin_array:
                std::cout << event.event_type() << " " << "\n";
                break;
            case staj_event_type::end_array:
                std::cout << event.event_type() << " " << "\n";
                break;
            case staj_event_type::begin_object:
                std::cout << event.event_type() << " " << "\n";
                break;
            case staj_event_type::end_object:
                std::cout << event.event_type() << " " << "\n";
                break;
            case staj_event_type::key:
                // Or std::string_view, if supported
                std::cout << event.event_type() << ": " << event.get<jsoncons::string_view>() << "\n";
                break;
            case staj_event_type::string_value:
                // Or std::string_view, if supported
                std::cout << event.event_type() << ": " << event.get<jsoncons::string_view>() << "\n";
                break;
            case staj_event_type::null_value:
                std::cout << event.event_type() << "\n";
                break;
            case staj_event_type::bool_value:
                std::cout << event.event_type() << ": " << std::boolalpha << event.get<bool>() << "\n";
                break;
            case staj_event_type::int64_value:
                std::cout << event.event_type() << ": " << event.get<int64_t>() << "\n";
                break;
            case staj_event_type::uint64_value:
                std::cout << event.event_type() << ": " << event.get<uint64_t>() << "\n";
                break;
            case staj_event_type::double_value:
                std::cout << event.event_type() << ": " << event.get<double>() << "\n";
                break;
            default:
                std::cout << "Unhandled event type: " << event.event_type() << " " << "\n";
                break;
        }
    }
}

void grouped_into_basic_json_records()
{
    csv::csv_options options;
    options.assume_header(true);

    csv::csv_string_cursor cursor(data, options);

    auto view = staj_array<ojson>(cursor);
    auto it = view.begin();
    auto end = view.end();

    json_options print_options;
    print_options.float_format(float_chars_format::fixed);
    while (it != end)
    {
        std::cout << pretty_print(*it, print_options) << "\n";
        ++it;
    }
}

void grouped_into_strongly_typed_records()
{
    using record_type = std::tuple<std::string,std::string,double>;

    csv::csv_options options;
    options.assume_header(true);
    csv::csv_string_cursor cursor(data, options);

    auto view = staj_array<record_type>(cursor);

    std::cout << std::fixed << std::setprecision(7);
    for (const auto& record : view)
    {
        std::cout << std::get<0>(record) << ", " << std::get<1>(record) << ", " << std::get<2>(record) << "\n";
    }
}

int main()
{
    std::cout << "\nCSV examples\n\n";
    read_write_csv_tasks();
    encode_to_tab_delimited_file();
    serialize_array_of_arrays_to_comma_delimited();
    serialize_books_to_csv_file();
    serialize_books_to_csv_file_with_reorder();
    last_column_repeats();
    last_two_columns_repeat();
    decode_csv_string();
    decode_csv_stream();
    encode_csv_file_from_books();
    decode_encode_csv_tasks();

    csv_decode_without_type_inference();
    csv_parser_type_inference();

    decode_csv_with_subfields();
    csv_source_to_json_value();

    std::cout << "\n";
    as_a_variant_like_structure();
    std::cout << "\n";
    as_a_strongly_typed_cpp_structure();
    std::cout << "\n";
    as_a_stream_of_json_events();
    std::cout << "\n";
    grouped_into_basic_json_records();
    std::cout << "\n";
    grouped_into_strongly_typed_records();
    std::cout << "\n";
    encode_n_objects();
    std::cout << "\n";
    encode_n_rows();
    std::cout << "\n";
    encode_m_columns();
    std::cout << "\n";

    csv_source_to_cpp_object();

    std::cout << std::endl;
}

