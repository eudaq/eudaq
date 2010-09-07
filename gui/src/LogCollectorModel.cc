#include "eudaq/Utils.hh"
#include "eudaq/Exception.hh"
#include "LogCollectorModel.hh"

#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <set>
#include <algorithm>

using eudaq::to_string;
using eudaq::from_string;
using eudaq::firstline;
using eudaq::split;

namespace {

  static eudaq::LogMessage make_msg(const std::string & fileline) {
     std::vector<std::string> parts = split(fileline);
//     std::cout << "line: " << fileline << std::endl;
//     for (size_t i = 0; i < parts.size(); ++i) {
//       std::cout << " " << i << ": \'" << parts[i] << "\'" << std::endl;
//     }
    if (parts.size() == 1) {
      parts = split(fileline, " ");
      if (parts.size() == 7 && parts[0] == "***" && parts[3] == "at" && parts[6] == "***") {
        std::vector<std::string> timeparts = split(parts[4], "-");
        std::vector<std::string> tempparts = split(parts[5], ":.");
        if (timeparts.size() != 3 || tempparts.size() != 4) EUDAQ_THROW("Badly formatted time: " + parts[4] + " " + parts[5]);
        timeparts.insert(timeparts.end(), tempparts.begin(), tempparts.end());
//         std::vector<std::string> & r = timeparts;
//         for (size_t i = 0; i < r.size(); ++i) {
//           std::cout << i << ": \'" << r[i] << "\' " << from_string(r[i], 0) << std::endl;
//         }
        eudaq::Time time(from_string(timeparts[0], 1970), from_string(timeparts[1], 1), from_string(timeparts[2], 1),
                         from_string(timeparts[3], 0), from_string(timeparts[4], 0), from_string(timeparts[5], 0),
                         from_string(timeparts[6], 0) * 1000);
        return eudaq::LogMessage("*** " + parts[2] + " ***", eudaq::Status::LVL_USER, time)
          .SetSender(parts[1]);
      }
    } else if (parts.size() == 6) {
      std::vector<std::string> timeparts = split(parts[2], " -.:");
      if (timeparts.size() != 7) EUDAQ_THROW("Badly formatted time: " + parts[2]);
      int level = 0;
      while (eudaq::Status::Level2String(level) != "" &&
             eudaq::Status::Level2String(level) != parts[0]) {
        level++;
      }
      eudaq::Time time(from_string(timeparts[0], 1970), from_string(timeparts[1], 1), from_string(timeparts[2], 1),
                       from_string(timeparts[3], 0), from_string(timeparts[4], 0), from_string(timeparts[5], 0),
                       from_string(timeparts[6], 0) * 1000);
      size_t colon = parts[4].find_last_of(":");
      return eudaq::LogMessage(parts[1], (eudaq::Status::Level)level, time)
        .SetLocation(parts[4].substr(0, colon),
                     from_string(parts[4].substr(colon+1), 0),
                     parts[5])
        .SetSender(parts[3]);
    }
    EUDAQ_THROW("Badly formatted log message: " + fileline);
  }

  static std::string simple_file(const std::string & file) {
    std::string result = file;
    size_t i;
    while ((i = result.find_first_of("/:\\")) != std::string::npos) {
      if (result[i] == ':' && result.find_first_not_of("0123456789") != std::string::npos) {
        break;
      }
      result = result.substr(i+1);
    }
    return result;
  }

  static std::string simple_func(const std::string & func) {
    std::string result = func;
    size_t i, start = 0;
    while ((i = result.find_first_of(" (:", start)) != std::string::npos) {
      if (result[i] == '(') {
        break;
      } else if (result[i] == ' ') {
        result = result.substr(i+1);
        start = 0;
      } else if (result[i] == ':') {
        if (result[i+1] == ':') {
          result = result.substr(i+2);
          start = 0;
        } else {
          start = i+1;
        }
      }
    }
    return result;
  }

}

LogMessage::LogMessage(const eudaq::LogMessage & msg) : eudaq::LogMessage(msg), m_ok(true) {}
LogMessage::LogMessage(const std::string & fileline)  : eudaq::LogMessage(make_msg(fileline)) {}

namespace {
  static const char * const g_columns[] = {
    "Received",
    "Sent",
    "Level",
    "Text",
    "From",
    "File",
    "Function"
  };
}

int LogMessage::ColumnWidth(int i) {
  switch (i) {
  case 0:
  case 1: return 100;
  case 2: return 60;
  case 3: return 400;
  default: return -1;
  }
}

QString LogMessage::operator [] (int i) const {
  switch(i) {
  case 0: return m_createtime.Formatted("%H:%M:%S.%3").c_str();
  case 1: return m_time.Formatted("%H:%M:%S.%3").c_str();
  case 3: return firstline(this->Text(i)).c_str();
  case 5: return simple_file(this->Text(i)).c_str();
  case 6: return simple_func(this->Text(i)).c_str();
  default: return this->Text(i).c_str();
  }
}

std::string LogMessage::Text(int i) const {
  switch(i) {
  case 0: return m_createtime.Formatted("%Y-%m-%d %H:%M:%S.%6");
  case 1: return m_time.Formatted("%Y-%m-%d %H:%M:%S.%6");
  case 2: return (to_string(m_level) + "-" + Level2String(m_level));
  case 3: return m_msg;
  case 4: return GetSender();
  case 5: return m_file + (m_line == 0 ? "" : (":" + to_string(m_line)));
  case 6: return m_func;
  default: return "";
  }
}

int LogMessage::NumColumns() {
  return sizeof g_columns / sizeof *g_columns;
}

const char * LogMessage::ColumnName(int i) {
  if (i < 0 || i >= NumColumns()) {
    return "";
  }
  return g_columns[i];
}

void LogSearcher::SetSearch(const std::string & regexp) {
  m_set = (regexp != "");
  m_regexp.setPattern(regexp.c_str());
}

bool LogSearcher::Match(const LogMessage & msg) {
  if (!m_set) return true;
  for (int i = 0; i < LogMessage::NumColumns(); ++i) {
    if (m_regexp.indexIn(msg[i]) >= 0) return true;
  }
  return false;
}

LogCollectorModel::LogCollectorModel(QObject *parent)
  : QAbstractListModel(parent),
    m_displaylevel(0),
    m_sorter(&m_all)
{
}

std::vector<std::string> LogCollectorModel::LoadFile(const std::string & filename) {
  std::ifstream file(filename.c_str());
  if (!file.is_open()) throw std::runtime_error("Unable to open file " + filename);
  std::set<std::string> sources;
  while (!file.eof() && !file.bad()) {
    std::string line;
    getline(file, line);
    if (line.length() > 0) {
      LogMessage msg(line);
      AddMessage(msg);
      sources.insert(msg.GetSender());
    }
  }
  return std::vector<std::string>(sources.begin(), sources.end());
}

bool LogCollectorModel::IsDisplayed(size_t index) {
  LogMessage & msg = m_all[index];
  return (msg.GetLevel() >= m_displaylevel &&
          (m_displaytype == "" || m_displaytype == "All" || msg.GetSenderType() == m_displaytype) &&
          (m_displayname == "" || m_displayname == "*"   || msg.GetSenderName() == m_displayname) &&
          m_search.Match(msg));
}

QModelIndex LogCollectorModel::AddMessage(const LogMessage & msg) {
  m_all.push_back(msg);
  if (IsDisplayed(m_all.size() - 1)) {
    std::vector<size_t>::iterator it = std::lower_bound(m_disp.begin(), m_disp.end(), m_all.size() - 1, m_sorter);
    size_t pos = it - m_disp.begin();
    beginInsertRows(QModelIndex(), pos, pos);
    m_disp.insert(it, m_all.size() - 1);
    endInsertRows();
    //std::cout << "Added row=" << pos << std::endl;
    return createIndex(pos, 0);
  }
  return QModelIndex();
}

void LogCollectorModel::UpdateDisplayed() {
  if (m_disp.size() > 0) {
    beginRemoveRows(createIndex(0, 0), 0, m_disp.size() - 1);
    m_disp.clear();
    endRemoveRows();
  }
  std::vector<size_t> disp;
  for (size_t i = 0; i < m_all.size(); ++i) {
    if (IsDisplayed(i)) {
      disp.push_back(i);
    }
  }
  std::sort(disp.begin(), disp.end(), m_sorter);
  if (disp.size() > 0) {
    beginInsertRows(createIndex(0, 0), 0, disp.size() - 1);
    m_disp = disp;
    endInsertRows();
  }
}

// void LogCollectorModel::newconnection(const eudaq::ConnectionInfo & id) {
//   emit layoutAboutToBeChanged();
//   m_data.push_back(LogCollectorConnection(id));
//   emit layoutChanged();
// }

int LogCollectorModel::rowCount(const QModelIndex &/*parent*/) const {
  return m_disp.size();
}

int LogCollectorModel::columnCount(const QModelIndex &/*parent*/) const {
  return LogMessage::NumColumns();
}

int LogCollectorModel::GetLevel(const QModelIndex &index) const {
  return m_all[m_disp[index.row()]].GetLevel();
}

QVariant LogCollectorModel::data(const QModelIndex &index, int role) const {
  if (role != Qt::DisplayRole || !index.isValid())
    return QVariant();

  if (index.column() < columnCount() && index.row() < rowCount())
    return GetMessage(index.row())[index.column()];

  return QVariant();
}

const LogMessage & LogCollectorModel::GetMessage(int row) const {
  return m_all[m_disp[row]];
}

QVariant LogCollectorModel::headerData(int section, Qt::Orientation orientation,
                                      int role) const {
  if (role != Qt::DisplayRole)
    return QVariant();

  if (orientation == Qt::Horizontal && section < columnCount())
    return LogMessage::ColumnName(section);

  return QVariant();
}

