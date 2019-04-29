/**
 * Autogenerated by Thrift Compiler (0.9.3)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#include "defines_types.h"

#include <algorithm>
#include <ostream>

#include <thrift/TToString.h>

namespace scatsService {


T_Datetime::~T_Datetime() throw() {
}


void T_Datetime::__set_iYear(const int16_t val) {
  this->iYear = val;
}

void T_Datetime::__set_byMonth(const int8_t val) {
  this->byMonth = val;
}

void T_Datetime::__set_byDay(const int8_t val) {
  this->byDay = val;
}

void T_Datetime::__set_byHour(const int8_t val) {
  this->byHour = val;
}

void T_Datetime::__set_byMinute(const int8_t val) {
  this->byMinute = val;
}

void T_Datetime::__set_bySecond(const int8_t val) {
  this->bySecond = val;
}

uint32_t T_Datetime::read(::apache::thrift::protocol::TProtocol* iprot) {

  apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
  uint32_t xfer = 0;
  std::string fname;
  ::apache::thrift::protocol::TType ftype;
  int16_t fid;

  xfer += iprot->readStructBegin(fname);

  using ::apache::thrift::protocol::TProtocolException;


  while (true)
  {
    xfer += iprot->readFieldBegin(fname, ftype, fid);
    if (ftype == ::apache::thrift::protocol::T_STOP) {
      break;
    }
    switch (fid)
    {
      case 1:
        if (ftype == ::apache::thrift::protocol::T_I16) {
          xfer += iprot->readI16(this->iYear);
          this->__isset.iYear = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 2:
        if (ftype == ::apache::thrift::protocol::T_BYTE) {
          xfer += iprot->readByte(this->byMonth);
          this->__isset.byMonth = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 3:
        if (ftype == ::apache::thrift::protocol::T_BYTE) {
          xfer += iprot->readByte(this->byDay);
          this->__isset.byDay = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 4:
        if (ftype == ::apache::thrift::protocol::T_BYTE) {
          xfer += iprot->readByte(this->byHour);
          this->__isset.byHour = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 5:
        if (ftype == ::apache::thrift::protocol::T_BYTE) {
          xfer += iprot->readByte(this->byMinute);
          this->__isset.byMinute = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 6:
        if (ftype == ::apache::thrift::protocol::T_BYTE) {
          xfer += iprot->readByte(this->bySecond);
          this->__isset.bySecond = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      default:
        xfer += iprot->skip(ftype);
        break;
    }
    xfer += iprot->readFieldEnd();
  }

  xfer += iprot->readStructEnd();

  return xfer;
}

uint32_t T_Datetime::write(::apache::thrift::protocol::TProtocol* oprot) const {
  uint32_t xfer = 0;
  apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
  xfer += oprot->writeStructBegin("T_Datetime");

  xfer += oprot->writeFieldBegin("iYear", ::apache::thrift::protocol::T_I16, 1);
  xfer += oprot->writeI16(this->iYear);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("byMonth", ::apache::thrift::protocol::T_BYTE, 2);
  xfer += oprot->writeByte(this->byMonth);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("byDay", ::apache::thrift::protocol::T_BYTE, 3);
  xfer += oprot->writeByte(this->byDay);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("byHour", ::apache::thrift::protocol::T_BYTE, 4);
  xfer += oprot->writeByte(this->byHour);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("byMinute", ::apache::thrift::protocol::T_BYTE, 5);
  xfer += oprot->writeByte(this->byMinute);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("bySecond", ::apache::thrift::protocol::T_BYTE, 6);
  xfer += oprot->writeByte(this->bySecond);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}

void swap(T_Datetime &a, T_Datetime &b) {
  using ::std::swap;
  swap(a.iYear, b.iYear);
  swap(a.byMonth, b.byMonth);
  swap(a.byDay, b.byDay);
  swap(a.byHour, b.byHour);
  swap(a.byMinute, b.byMinute);
  swap(a.bySecond, b.bySecond);
  swap(a.__isset, b.__isset);
}

T_Datetime::T_Datetime(const T_Datetime& other0) {
  iYear = other0.iYear;
  byMonth = other0.byMonth;
  byDay = other0.byDay;
  byHour = other0.byHour;
  byMinute = other0.byMinute;
  bySecond = other0.bySecond;
  __isset = other0.__isset;
}
T_Datetime& T_Datetime::operator=(const T_Datetime& other1) {
  iYear = other1.iYear;
  byMonth = other1.byMonth;
  byDay = other1.byDay;
  byHour = other1.byHour;
  byMinute = other1.byMinute;
  bySecond = other1.bySecond;
  __isset = other1.__isset;
  return *this;
}
void T_Datetime::printTo(std::ostream& out) const {
  using ::apache::thrift::to_string;
  out << "T_Datetime(";
  out << "iYear=" << to_string(iYear);
  out << ", " << "byMonth=" << to_string(byMonth);
  out << ", " << "byDay=" << to_string(byDay);
  out << ", " << "byHour=" << to_string(byHour);
  out << ", " << "byMinute=" << to_string(byMinute);
  out << ", " << "bySecond=" << to_string(bySecond);
  out << ")";
}

} // namespace
