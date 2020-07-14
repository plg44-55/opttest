#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <set>
#include <string.h>
#include <OB/CORBA.h>
#include <OB/Codec.h>
#include <OB/DynamicAny.h>
#include "main_data.h"

template<class S>
void
spush(S& seq, typename S::ConstSubType el)
{
    //static_assert(std::is_same<typename S::SubType, E&>::value, "Types don't equal.");
    CORBA::ULong len = seq.length();
    seq.length(len+1);
    seq[len] = el;
}

namespace MINC
{
    template<class S>
    typename std::remove_reference<typename S::ConstSubType>::type*
    begin(const S& s)
    {
	return s.get_buffer();
    }

    template<class S>
    typename std::remove_reference<typename S::SubType>::type*
    begin(S& s)
    {
	return s.get_buffer();
    }

    template<class S>
    typename std::remove_reference<typename S::ConstSubType>::type*
    end(const S& s)
    {
	return s.get_buffer()+s.length();
    }

    template<class S>
    typename std::remove_reference<typename S::ConstSubType>::type*
    end(S& s)
    {
	return s.get_buffer()+s.length();
    }
}

struct Error
{
    std::string msg;
    Error(const char* m) : msg(m) { }
};

CORBA::ORB_var m_orb;

const std::map<CORBA::TCKind, const char*> kind_names
	{
	    { CORBA::tk_boolean, "boolean" },
	    { CORBA::tk_octet, "octet" },
	    { CORBA::tk_short, "short" },
	    { CORBA::tk_ushort, "ushort" },
	    { CORBA::tk_long, "long" },
	    { CORBA::tk_ulong, "ulong" },
	    { CORBA::tk_longlong, "longlong" },
	    { CORBA::tk_ulonglong, "ulonglong" },
	    { CORBA::tk_float, "float" },
	    { CORBA::tk_double, "double" },
	    { CORBA::tk_longdouble, "longdouble" },
	    { CORBA::tk_char, "char" },
	    { CORBA::tk_string, "string" },
	    { CORBA::tk_objref, "reference" },
	    { CORBA::tk_any, "any" },
	    { CORBA::tk_array, "array" },
	    { CORBA::tk_sequence, "sequence" },
	    { CORBA::tk_enum, "enum" },
	    { CORBA::tk_struct, "struct" },
	    { CORBA::tk_union, "union" },
	    { CORBA::tk_alias, "alias" }
	};

std::ostream&
operator<<(std::ostream& s, CORBA::TCKind k)
{
    return s << kind_names.at(k);
}

char*
tc2NameId(CORBA::TypeCode_ptr tc)
{
    CORBA::String_var _r = CORBA::string_dup("");
    try
    {
	if( CORBA::is_nil(tc) )
	{
	    _r+= "_nil";
	    return _r._retn();
	}
	switch( tc->kind() )
	{
	case CORBA::tk_boolean:		_r += "boolean"; break;
	case CORBA::tk_octet:		_r += "octet"; break;
	case CORBA::tk_short:		_r += "short"; break;
	case CORBA::tk_ushort:		_r += "ushort"; break;
	case CORBA::tk_long:		_r += "long"; break;
	case CORBA::tk_ulong:		_r += "ulong"; break;
	case CORBA::tk_longlong:	_r += "longlong"; break;
	case CORBA::tk_ulonglong:	_r += "ulonglong"; break;
	case CORBA::tk_float:		_r += "float"; break;
	case CORBA::tk_double:		_r += "double"; break;
	case CORBA::tk_longdouble:	_r += "longdouble"; break;
	case CORBA::tk_char:		_r += "char"; break;
	case CORBA::tk_string:		_r += "string"; break;
	case CORBA::tk_objref:		_r += "reference"; break;
	case CORBA::tk_any:		_r += "any"; break;
	case CORBA::tk_array:		_r += "array"; break;
	case CORBA::tk_sequence:	_r += "sequence"; break;
	case CORBA::tk_enum:
	case CORBA::tk_struct:
	case CORBA::tk_union:
	case CORBA::tk_alias:
	{
	    _r	+=  tc->name();
	    _r	+=  " - ";
	    _r	+=  tc->id();
	}
	break;
	default: _r += "unknown"; break;
	}
    }
    catch(...) {}
    return _r._retn();
}

CORBA::OctetSeq*
getStreamFromFile(const char* fname)
{
    CORBA::OctetSeq* s = new CORBA::OctetSeq;
    try
    {
	std::ifstream f(fname, std::ios_base::in | std::ios_base::binary);

	char c;
	f.get(c);
	while( !f.fail() && !f.eof() )
	{
	    spush(*s, CORBA::Octet(c));
	    f.get(c);
	}

	std::cout << "getStreamFromFile: readed " << s->length() << " bytes." << std::endl;
    }
    catch(...)
    {
	std::cout << "getStreamFromFile: exception." << std::endl;
    }
    return s;
}

CORBA::Any*
getAnyFromStream(const CORBA::OctetSeq& s)
{
    CORBA::Any_var a = new CORBA::Any;
    try
    {
	if(s.length() == 0)
	    throw Error("Zero length");

	CORBA::Object_var obj = m_orb->resolve_initial_references("CodecFactory");
	if(CORBA::is_nil(obj))
	    throw Error("can't resolve CodecFactory");

	IOP::CodecFactory_var codec_factory = IOP::CodecFactory::_narrow(obj);
	if(CORBA::is_nil(codec_factory))
	    throw Error("can't narrow CodecFactory");

	IOP::Encoding how;
	how.major_version = 1;
	how.minor_version = 0;
	how.format = IOP::ENCODING_CDR_ENCAPS;

	IOP::Codec_var cdr_codec = codec_factory->create_codec(how);
	if(CORBA::is_nil(cdr_codec))
	    throw Error("can't create codec");

	a = cdr_codec->decode(s);
	std::cout << "getAnyFromStream: decoding stream to any done." << std::endl;
    }
    catch(const Error& e)
    {
	std::cout << e.msg << std::endl;
    }
    catch(const CORBA::Exception& e)
    {
	std::cout << "getAnyFromStream: decoding stream to any failed:";
	std::cout << e;
    }
    catch(...)
    {
	std::cout << "getAnyFromStream: decoding stream to any failed!" << std::endl;
    }
    return a._retn();
}

CORBA::Any*
getAnyFromFile(const char* fileName)
{
    CORBA::Any_var any = new CORBA::Any;
    try
    {
	CORBA::OctetSeq_var f_stream = getStreamFromFile(fileName);
	any = getAnyFromStream(f_stream);

	std::cout << "getAnyFromFile: done. File name = " << fileName
		  << ", TC name - id = " << CORBA::String_var(tc2NameId(CORBA::TypeCode_var(any->type()))) << "." << std::endl;
    }
    catch(...)
    {
	std::cout << "getAnyFromFile: exception." << std::endl;
    }
    return any._retn();
}

struct TypeCodePrefixPair
{
    CORBA::TypeCode_ptr tc;
    const char* pr;
};


TypeCodePrefixPair
getAnyTypeAndPrefix(const char* fileName)
{
    TypeCodePrefixPair tc_pref { CORBA::TypeCode::_nil(), "" };
    try
    {
	CORBA::OctetSeq_var f_stream = getStreamFromFile(fileName);
	CORBA::Any_var f_any = getAnyFromStream(f_stream);
	tc_pref.tc = f_any->type();
	if( tc_pref.tc->kind() == CORBA::tk_null )
	    return tc_pref;

	CORBA::String_var tc_id = CORBA::string_dup(tc_pref.tc->id());
	char* p = strstr(tc_id, "IDL:");
	if(p != nullptr)
	{
	    p += 4;
	    strtok(p, "/");
	    tc_pref.pr = CORBA::string_dup(p);
	}
	std::cout << "getAnyTypeAndPrefixFromFile: done. TC name - id = " << CORBA::String_var(tc2NameId(tc_pref.tc))
		<< ", prefix = " << tc_pref.pr << ".\n";
    }
    catch(...)
    {
	std::cout << "UNKNOWN EXCEPTION getAnyTypeAndPrefixFromFile\n";
	tc_pref = { CORBA::TypeCode::_nil(), "" };
    }
    return tc_pref;
}

TypeCodePrefixPair
getAnyTypeAndPrefix(const CORBA::Any& any)
{
    TypeCodePrefixPair tc_pref { CORBA::TypeCode::_nil(), "" };
    try
    {
	tc_pref.tc = any.type();

	CORBA::String_var tc_id = CORBA::string_dup(tc_pref.tc->id());
	char* p = strstr(tc_id, "IDL:");
	if(p != nullptr)
	{
	    p += 4;
	    strtok(p, "/");
	    tc_pref.pr = CORBA::string_dup(p);
	}
	std::cout << "getAnyTypeAndPrefixFromAny: done. TC name - id = " << CORBA::String_var(tc2NameId(tc_pref.tc))
	        << ", prefix = " << tc_pref.pr << ".\n";
    }
    catch(...)
    {
	std::cout << "UNKNOWN EXCEPTION getAnyTypeAndPrefixFromAny\n";
	tc_pref = { CORBA::TypeCode::_nil(), "" };
    }
    return tc_pref;
}

void
setAnyToFile(const char* fileName, const CORBA::Any& any)
{
    std::ofstream f;
    try
    {
	f.open(fileName, std::ios_base::out | std::ios_base::binary);

	CORBA::Object_var obj = m_orb->resolve_initial_references("CodecFactory");
	if(CORBA::is_nil(obj))
	    throw Error("can't resolve CodecFactory");

	IOP::CodecFactory_var codec_factory = IOP::CodecFactory::_narrow(obj);
	if(CORBA::is_nil(codec_factory))
	    throw Error("can't narrow CodecFactory");

	IOP::Encoding how;
	how.major_version = 1;
	how.minor_version = 0;
	how.format = IOP::ENCODING_CDR_ENCAPS;

	IOP::Codec_var cdr_codec = codec_factory->create_codec(how);
	if(CORBA::is_nil(cdr_codec))
	    throw Error("can't create codec");

	CORBA::OctetSeq_var os = cdr_codec->encode(any);
	if(os->length() == 0)
	    throw Error("can't encode");

	f.write((const char*)os->get_buffer(), os->length());
	f.flush();
	f.close();

	std::cout << "setAnyToFile: done. File name = " << fileName
		  << ", TC name - id = " << CORBA::String_var(tc2NameId(CORBA::TypeCode_var(any.type()))) << ".\n";
    }
    catch(const std::ios_base::failure& e)
    {
        std::cout << "Writing " << fileName << "Caught an ios_base::failure.\n"
                  << "Explanatory string: " << e.what() << '\n';
	    //<< "Error code: " << e.code() << '\n';
    }
    catch(...)
    {
	std::cout << "UNKNOWN EXCEPTION setAnyToFile\n";
    }

    if( f.is_open() )
    {
	f.flush();
	f.close();
    }
}

const std::map<MINC::STEP, const char*> step_rep = { //{ MINC::subzero, "zero" },
						     { MINC::zero, "zero" },
						     { MINC::five, "rfive" }, };
						     //{ MINC::six, "rsix" },
						     //{ MINC::seven, "rseven" } };

std::ostream&
operator<<(std::ostream& s, MINC::STEP d)
{
    return s << step_rep.at(d);
}

std::ostream&
operator<<(std::ostream& s, const MMAIN::main_data& d)
{
    s << "MMAIN::main_data\nversion: " << d.version << std::endl;
    for(auto& e : d.data)
	s << e.data_key << " / " << e.desc.in() << " / " << e.stp << std::endl;
    return s;
}

void
compare(const char* fname, const CORBA::Any& md_any);

int
main(int argc, char* argv[])
{
    // create & save main_data_1_0 + included_data_1_0
    // update idl : main_data_1_0 + included_data_1_1
    // compare

    m_orb = CORBA::ORB_init(argc, argv);
    MMAIN::main_data md;
    CORBA::String_var fname = "optns";

    try
    {
	CORBA::Any md_any;
	bool is_write = (argc > 1 && strcmp(argv[1], "-cr") == 0);

	// create
	if( is_write )
	{
	    md.version = 5;
	    md.data.length(3);
	    md.data[0].data_key = 0x77;
	    md.data[0].desc = "one";
	    md.data[0].stp = MINC::five;
	    md.data[1].data_key = 0x77;
	    md.data[1].desc = "two";
	    md.data[1].stp = MINC::five;
	    md.data[2].data_key = 0x77;
	    md.data[2].desc = "this is very very very looooong stringgg!";
	    md.data[2].stp = MINC::five;
	}
	else
	{
	    md.version = 11;
	}

	md_any <<= md;

	if( is_write )
	{
	    setAnyToFile(fname, md_any);
	    std::cout << "Options saved in " << fname.in() << ".\n";
	    return 0;
	}

	TypeCodePrefixPair fds = getAnyTypeAndPrefix(fname);
	TypeCodePrefixPair ds = getAnyTypeAndPrefix(md_any);

	if( CORBA::is_nil(fds.tc) || CORBA::is_nil(ds.tc) )
	{
	    std::cout << "type(s) is(are) nil!" << std::endl;
	    return 1;
	}

	bool types_equal = ds.tc->equal(fds.tc);
	bool types_equal1 = fds.tc->equal(ds.tc);
	bool types_equivalent = ds.tc->equivalent(fds.tc);
	bool types_equivalent1 = fds.tc->equivalent(ds.tc);
	bool prefixes_equal = (strcmp(ds.pr, fds.pr) == 0);
	std::cout << "file options prefix = " << fds.pr
		  << ", current options prefix = " << ds.pr << "." << std::endl
		  << " types " << (types_equal ? "equal" : "not equal") << std::endl
		  << " types1 " << (types_equal ? "equal" : "not equal") << std::endl
		  << " types " << (types_equivalent ? "equivalent" : "not equivalent") << std::endl
		  << " types1 " << (types_equivalent1 ? "equivalent" : "not equivalent") << std::endl
		  << " prefixes " << (prefixes_equal ? "equal" : "not equal")
		  << std::endl;

	std::cout << std::endl;
	compare(fname, md_any);
	std::cout << std::endl;

	if( types_equal )
	{
	    CORBA::Any_var val = getAnyFromFile(fname);
	    const MMAIN::main_data* rmd;
	    val >>= rmd;
	    std::cout << "///////////\n" << *rmd << std::endl;
	}
    }
    catch(const Error& e)
    {
	std::cout << e.msg << std::endl;
    }
    catch(...)
    {
	std::cout << "Exception in main." << std::endl;
    }

}

struct any_tree_elt
{
    unsigned level;
    CORBA::TCKind kind;
    std::string id;
    std::string field_name;
    std::string attr;
    any_tree_elt(unsigned l, CORBA::TCKind k, std::string _id, std::string _fn, std::string s) :
	level(l), kind(k), id(_id), field_name(_fn), attr(s) { }
};

using any_tree = std::vector<any_tree_elt>;

void
show_tree(const any_tree&);

any_tree
get_tree(DynamicAny::DynAny_ptr, unsigned l = 0, std::string n = "--");

void
fill_new(DynamicAny::DynAny_ptr a_old, const any_tree& tree, DynamicAny::DynAny_ptr a_new, unsigned l = 0);

void
compare(const char* fname, const CORBA::Any& current_any)
{
    try
    {
	CORBA::OctetSeq_var f_stream = getStreamFromFile(fname);
	CORBA::Any_var f_any = getAnyFromStream(f_stream);

	CORBA::Object_var factory_obj = m_orb->resolve_initial_references("DynAnyFactory");
	DynamicAny::DynAnyFactory_var factory = DynamicAny::DynAnyFactory::_narrow(factory_obj);

	DynamicAny::DynAny_var dyn_any = factory->create_dyn_any(current_any);
	any_tree new_tree = get_tree(dyn_any);
	show_tree(new_tree);

	std::cout << "----------------------------------\n";

	DynamicAny::DynAny_var dyn_any1 = factory->create_dyn_any(f_any);
	//DynamicAny::DynAny_var dyn_any1 = factory->create_dyn_any_without_truncation(f_any);

	// any_tree old_tree = get_tree(dyn_any1);
	// show_tree(old_tree);

	// may be:
	// class c_dyn_any:
	//   c_dyn_any(dyn_any)
	//   c_dyn_any.get_tree()
	//   c_dyn_any.fill_new(dyn_any1);
	//   dyn_any = c_dyn_any.get_filled();
	fill_new(dyn_any, new_tree, dyn_any1);
	// compare
    }
    catch(...)
    {
	std::cout << "error in compare\n";
    }
}

std::string
get_id(CORBA::TypeCode_ptr tc)
{
    using namespace CORBA;
    const std::set<TCKind> can { tk_objref, tk_struct, tk_union, tk_enum, tk_alias, tk_except };
    std::string _r = can.count(tc->kind()) ? tc->id() : " -- ";
    return _r;
}

any_tree
get_tree(DynamicAny::DynAny_ptr a, unsigned level, std::string field_name)
{
    std::cout << "enter level " << level;
    a->rewind();
    std::ostringstream ostr_;
    CORBA::TypeCode_var tc = a->type();
    std::string id = get_id(tc);
    std::cout << " kind " << tc->kind() << ", " << id << " /// ";
    tc = OB::GetOrigType(tc);
    CORBA::TCKind kind = tc->kind();
    std::cout << " orig kind " << kind << ", " << get_id(tc) << std::endl;
    // while( kind == CORBA::tk_alias )
    // {
    // 	ostr_ << ' ' << tc -> name() << ';';
    // 	tc = tc -> content_type();
    // 	kind = tc->kind();
    // }

    any_tree at;

    switch(kind)
    {
    case CORBA::tk_objref:
    {
        ostr_ << tc -> id();
	CORBA::Object_ptr v_obj = a->get_reference();
        if(CORBA::is_nil(v_obj))
            ostr_ << "<NULL>";

	break;
    }

    case CORBA::tk_short:
    {
        CORBA::Short v_short = a -> get_short();
        ostr_ << v_short;

	break;
    }

    case CORBA::tk_ushort:
    {
        CORBA::UShort v_ushort = a -> get_ushort();
        ostr_ << v_ushort;

	break;
    }

    case CORBA::tk_long:
    {
        CORBA::Long v_long = a -> get_long();
        ostr_ << v_long;

	break;
    }

    case CORBA::tk_ulong:
    {
        CORBA::ULong v_ulong = a -> get_ulong();
        ostr_ << v_ulong;

	break;
    }

    case CORBA::tk_longlong:
    {
        CORBA::LongLong v_longlong = a -> get_longlong();
        CORBA::String_var str = OB::LongLongToString(v_longlong);
        ostr_ << str;

	break;
    }

    case CORBA::tk_ulonglong:
    {
        CORBA::ULongLong v_ulonglong = a -> get_ulonglong();
        CORBA::String_var str = OB::ULongLongToString(v_ulonglong);
        ostr_ << str;

	break;
    }

    case CORBA::tk_float:
    {
        CORBA::Float v_float = a -> get_float();
        ostr_ << v_float;

	break;
    }

    case CORBA::tk_double:
    {
        CORBA::Double v_double = a -> get_double();
        ostr_ << v_double;

	break;
    }

#if SIZEOF_LONG_DOUBLE >= 12
    case CORBA::tk_longdouble:
    {
        CORBA::LongDouble v_longdouble = a -> get_longdouble();
        //
        // Not all platforms support this
        //
        //ostr_ << v_longdouble;
        char str[64];
        sprintf(str, "%31Lg", v_longdouble);
        ostr_ << str;

	break;
    }
#endif

    case CORBA::tk_boolean:
    {
        CORBA::Boolean v_boolean = a -> get_boolean();
        if(v_boolean == true)
	    ostr_ << "TRUE";
        else if(v_boolean == false)
	    ostr_ << "FALSE";
        else
	    ostr_ << "UNKNOWN_BOOLEAN";

	break;
    }

    case CORBA::tk_octet:
    {
        CORBA::Octet v_octet = a -> get_octet();
        ostr_ << static_cast<int>(v_octet);

	break;
    }

    case CORBA::tk_char:
    {
        CORBA::Char v_char = a -> get_char();
        ostr_ << "'" << static_cast<char>(v_char) << "'";

	break;
    }

    case CORBA::tk_string:
    {
        CORBA::String_var v_string = a -> get_string();
        ostr_ << '"' << v_string << '"';

	break;
    }

    case CORBA::tk_wchar:
    {
        CORBA::WChar v_wchar = a -> get_wchar();
	//
	// Output of narrowed wchar to narrow stream
	//
	ostr_ << "'" << static_cast<char>(v_wchar) << "' - narrowed";

	break;
    }

    case CORBA::tk_wstring:
    {
        CORBA::WString_var v_wstring = a -> get_wstring();
	//
	// Output of narrowed wstring to narrow stream
	//
	ostr_ << '"';
	for (CORBA::ULong i=0; i < OB::wcslen(v_wstring); i++)
	{
	    ostr_ << static_cast<char>(v_wstring[i]);
	}
	ostr_ <<  '"' << " - narrowed";

	break;
    }

    case CORBA::tk_TypeCode: {
        CORBA::TypeCode_var v_tc = a -> get_typecode();
        //show_name_TypeCode(v_tc);

	break;
    }

    case CORBA::tk_any:
    {
        CORBA::Any_var v_any = a -> get_any();
        //show_Any(v_any);
	ostr_ << "It's ANY";
	break;
    }

    case CORBA::tk_array:
    {
        DynamicAny::DynArray_var dyn_array = DynamicAny::DynArray::_narrow(a);
        // for(i = 0 ; i < tc -> length() ; i++)
	// {
        //     if(i != 0) ostr_ << ", ";
        //     DynAny_var component = dyn_array -> current_component();
	//     show_DynAny(component);
	//     dyn_array -> next();
	// }
	ostr_ << "It's Arrray, length: " << tc->length();

	break;
    }

    case CORBA::tk_sequence:
    {
        CORBA::ULong member_count = tc->length();
	CORBA::TypeCode_ptr itc = tc->content_type();
        DynamicAny::DynSequence_var dyn_sequence = DynamicAny::DynSequence::_narrow(a);
	ostr_ << "It's sequence, length: " << dyn_sequence->get_length();
	ostr_ << " alternative: " << member_count;
	ostr_ << " and type " << get_id(itc);
        for( CORBA::ULong i = 0 ; i < dyn_sequence -> get_length() ; i++)
	{
            DynamicAny::DynAny_var component = dyn_sequence -> current_component();
	    any_tree nested = get_tree(component, level+1);
	    at.insert(at.end(), nested.begin(), nested.end());

            dyn_sequence -> next();
	}

	break;
    }

    case CORBA::tk_enum:
    {
	DynamicAny::DynEnum_var dyn_enum = DynamicAny::DynEnum::_narrow(a);
	CORBA::String_var v_string = dyn_enum -> get_as_string();
	ostr_ << v_string;
	CORBA::ULong v_ul = dyn_enum -> get_as_ulong();
	ostr_ << " / " << v_ul;

	break;
    }

    case CORBA::tk_struct:
    {
        DynamicAny::DynStruct_var dyn_struct = DynamicAny::DynStruct::_narrow(a);

        CORBA::ULong member_count = tc -> member_count();
	ostr_ << "It's struct, members: " << member_count << " /// ";
        for(CORBA::ULong i = 0 ; i < member_count ; i++)
	{
	    CORBA::String_var member_name = dyn_struct -> current_member_name();
	    ostr_ << member_name << ", ";

	    DynamicAny::DynAny_var component = dyn_struct -> current_component();
	    std::cout << "field name: " << member_name << "    ";
	    any_tree nested = get_tree(component, level+1, member_name.in());
	    at.insert(at.end(), nested.begin(), nested.end());
            //show_DynAny(component);
            dyn_struct -> next();
        }

	break;
    }

    case CORBA::tk_union:
    {
        DynamicAny::DynUnion_var dyn_union = DynamicAny::DynUnion::_narrow(a);
        DynamicAny::DynAny_var component = dyn_union -> current_component();

        // skip() << "discriminator = ";
        // show_DynAny(component);
        // ostr_ << OB_ENDL;

        // if(dyn_union -> component_count() == 2)
        // {
        //     dyn_union -> next();

        //     component = dyn_union -> current_component();

        //     String_var member_name = dyn_union -> member_name();
        //     skip() << member_name << " = ";
        //     show_DynAny(component);

        //     ostr_ << OB_ENDL;
        // }
	ostr_ << "It's union, components: " << dyn_union->component_count();

	break;
    }

    case CORBA::tk_fixed:
    {
	DynamicAny::DynFixed_var dyn_fixed = DynamicAny::DynFixed::_narrow(a);

        CORBA::String_var str = dyn_fixed -> get_value();
	ostr_ << str;

	break;
    }

    // case CORBA::tk_alias:
    // {
    // 	ostr_ << "It's alias: " << tc->name();

    //     // TypeCode_var content_type = tc -> content_type();
    //     // show_name_TypeCode(content_type);
    //     // ostr_ << ' ' << tc -> name() << ';';

    // 	break;
    // }

    default:
    {
        assert(false);

        break;
    }
    }

    a -> rewind();

    any_tree_elt ate { level, kind, id, field_name, ostr_.str() };
    at.push_back(ate);
    return at;
}

void
show_tree(const any_tree& at)
{
    std::cout << "The TREE:\n";
    // for( auto& a : at )
    // 	std::cout << a.level << ", " << a.kind << " - " << a.attr << std::endl;
    for( auto a = at.rbegin(); a != at.rend(); ++a )
     	std::cout << a->level << ", " << a->kind << ", " << a->field_name << ", " << a->id << " === " << a->attr << std::endl;
}

void
fill_new(DynamicAny::DynAny_ptr a_new, const any_tree& tree, DynamicAny::DynAny_ptr a_old, unsigned level)
{
    std::cout << "filling enter level " << level;
    a_old->rewind();
    CORBA::TypeCode_var tc = a_old->type();
    std::cout << " kind " << tc->kind() << ", " << get_id(tc) << " /// ";
    tc = OB::GetOrigType(tc);
    CORBA::TCKind kind = tc->kind();
    std::cout << " orig kind " << kind << ", " << get_id(tc) << std::endl;

    // find current a_old in tree and place in a_new and old value in a_new
    // place = find_in_tree(tree, get_type_name_?Value?(a_old)
    // a_new.seek(place)
    // a_new.insert(Value)


    switch(kind)
    {
    case CORBA::tk_objref:
    {
        ostr_ << tc -> id();
	CORBA::Object_ptr v_obj = a->get_reference();
        if(CORBA::is_nil(v_obj))
            ostr_ << "<NULL>";

	break;
    }

    case CORBA::tk_short:
    {
        CORBA::Short v_short = a -> get_short();
        ostr_ << v_short;

	break;
    }

    case CORBA::tk_ushort:
    {
        CORBA::UShort v_ushort = a -> get_ushort();
        ostr_ << v_ushort;

	break;
    }

    case CORBA::tk_long:
    {
        CORBA::Long v_long = a -> get_long();
        ostr_ << v_long;

	break;
    }

    case CORBA::tk_ulong:
    {
        CORBA::ULong v_ulong = a -> get_ulong();
        ostr_ << v_ulong;

	break;
    }

    case CORBA::tk_longlong:
    {
        CORBA::LongLong v_longlong = a -> get_longlong();
        CORBA::String_var str = OB::LongLongToString(v_longlong);
        ostr_ << str;

	break;
    }

    case CORBA::tk_ulonglong:
    {
        CORBA::ULongLong v_ulonglong = a -> get_ulonglong();
        CORBA::String_var str = OB::ULongLongToString(v_ulonglong);
        ostr_ << str;

	break;
    }

    case CORBA::tk_float:
    {
        CORBA::Float v_float = a -> get_float();
        ostr_ << v_float;

	break;
    }

    case CORBA::tk_double:
    {
        CORBA::Double v_double = a -> get_double();
        ostr_ << v_double;

	break;
    }

#if SIZEOF_LONG_DOUBLE >= 12
    case CORBA::tk_longdouble:
    {
        CORBA::LongDouble v_longdouble = a -> get_longdouble();
        //
        // Not all platforms support this
        //
        //ostr_ << v_longdouble;
        char str[64];
        sprintf(str, "%31Lg", v_longdouble);
        ostr_ << str;

	break;
    }
#endif

    case CORBA::tk_boolean:
    {
        CORBA::Boolean v_boolean = a -> get_boolean();
        if(v_boolean == true)
	    ostr_ << "TRUE";
        else if(v_boolean == false)
	    ostr_ << "FALSE";
        else
	    ostr_ << "UNKNOWN_BOOLEAN";

	break;
    }

    case CORBA::tk_octet:
    {
        CORBA::Octet v_octet = a -> get_octet();
        ostr_ << static_cast<int>(v_octet);

	break;
    }

    case CORBA::tk_char:
    {
        CORBA::Char v_char = a -> get_char();
        ostr_ << "'" << static_cast<char>(v_char) << "'";

	break;
    }

    case CORBA::tk_string:
    {
        CORBA::String_var v_string = a -> get_string();
        ostr_ << '"' << v_string << '"';

	break;
    }

    case CORBA::tk_wchar:
    {
        CORBA::WChar v_wchar = a -> get_wchar();
	//
	// Output of narrowed wchar to narrow stream
	//
	ostr_ << "'" << static_cast<char>(v_wchar) << "' - narrowed";

	break;
    }

    case CORBA::tk_wstring:
    {
        CORBA::WString_var v_wstring = a -> get_wstring();
	//
	// Output of narrowed wstring to narrow stream
	//
	ostr_ << '"';
	for (CORBA::ULong i=0; i < OB::wcslen(v_wstring); i++)
	{
	    ostr_ << static_cast<char>(v_wstring[i]);
	}
	ostr_ <<  '"' << " - narrowed";

	break;
    }

    case CORBA::tk_TypeCode: {
        CORBA::TypeCode_var v_tc = a -> get_typecode();
        //show_name_TypeCode(v_tc);

	break;
    }

    case CORBA::tk_any:
    {
        CORBA::Any_var v_any = a -> get_any();
        //show_Any(v_any);
	ostr_ << "It's ANY";
	break;
    }

    case CORBA::tk_array:
    {
        DynamicAny::DynArray_var dyn_array = DynamicAny::DynArray::_narrow(a);
        // for(i = 0 ; i < tc -> length() ; i++)
	// {
        //     if(i != 0) ostr_ << ", ";
        //     DynAny_var component = dyn_array -> current_component();
	//     show_DynAny(component);
	//     dyn_array -> next();
	// }
	ostr_ << "It's Arrray, length: " << tc->length();

	break;
    }

    case CORBA::tk_sequence:
    {
        CORBA::ULong member_count = tc->length();
	CORBA::TypeCode_ptr itc = tc->content_type();
        DynamicAny::DynSequence_var dyn_sequence = DynamicAny::DynSequence::_narrow(a);
	ostr_ << "It's sequence, length: " << dyn_sequence->get_length();
	ostr_ << " alternative: " << member_count;
	ostr_ << " and type " << get_id(itc);
        for( CORBA::ULong i = 0 ; i < dyn_sequence -> get_length() ; i++)
	{
            DynamicAny::DynAny_var component = dyn_sequence -> current_component();
	    any_tree nested = get_tree(component, level+1);
	    at.insert(at.end(), nested.begin(), nested.end());

            dyn_sequence -> next();
	}

	break;
    }

    case CORBA::tk_enum:
    {
	DynamicAny::DynEnum_var dyn_enum = DynamicAny::DynEnum::_narrow(a);
	CORBA::String_var v_string = dyn_enum -> get_as_string();
	ostr_ << v_string;
	CORBA::ULong v_ul = dyn_enum -> get_as_ulong();
	ostr_ << " / " << v_ul;

	break;
    }

    case CORBA::tk_struct:
    {
        DynamicAny::DynStruct_var dyn_struct =
	    DynamicAny::DynStruct::_narrow(a);

        CORBA::ULong member_count = tc -> member_count();
	ostr_ << "It's struct, members: " << member_count << " /// ";
        for(CORBA::ULong i = 0 ; i < member_count ; i++)
	{
	    CORBA::String_var member_name = dyn_struct -> current_member_name();
	    ostr_ << member_name << ", ";
	    DynamicAny::DynAny_var component = dyn_struct -> current_component();
	    std::cout << "field name: " << member_name << "    ";
	    any_tree nested = get_tree(component, level+1);
	    at.insert(at.end(), nested.begin(), nested.end());
            //show_DynAny(component);
            dyn_struct -> next();
        }

	break;
    }

    case CORBA::tk_union:
    {
        DynamicAny::DynUnion_var dyn_union = DynamicAny::DynUnion::_narrow(a);
        DynamicAny::DynAny_var component = dyn_union -> current_component();

        // skip() << "discriminator = ";
        // show_DynAny(component);
        // ostr_ << OB_ENDL;

        // if(dyn_union -> component_count() == 2)
        // {
        //     dyn_union -> next();

        //     component = dyn_union -> current_component();

        //     String_var member_name = dyn_union -> member_name();
        //     skip() << member_name << " = ";
        //     show_DynAny(component);

        //     ostr_ << OB_ENDL;
        // }
	ostr_ << "It's union, components: " << dyn_union->component_count();

	break;
    }

    case CORBA::tk_fixed:
    {
	DynamicAny::DynFixed_var dyn_fixed = DynamicAny::DynFixed::_narrow(a);

        CORBA::String_var str = dyn_fixed -> get_value();
	ostr_ << str;

	break;
    }

    // case CORBA::tk_alias:
    // {
    // 	ostr_ << "It's alias: " << tc->name();

    //     // TypeCode_var content_type = tc -> content_type();
    //     // show_name_TypeCode(content_type);
    //     // ostr_ << ' ' << tc -> name() << ';';

    // 	break;
    // }

    default:
    {
        assert(false);

        break;
    }
    }

    a -> rewind();
}
