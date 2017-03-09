#include "RunControlModel.hh"

#include <vector>
#include <string>
#include "qmetatype.h"

RunControlModel::RunControlModel(QObject *parent)
  : QAbstractListModel(parent){
  qRegisterMetaType<QVector<int> >("QVector<int>");
}

void RunControlModel::newconnection(eudaq::ConnectionSPC id){
  m_con_status[id].reset();  
  beginInsertRows(QModelIndex(), 0, 0);
  endInsertRows();
  UpdateDisplayed();
}

void RunControlModel::disconnected(eudaq::ConnectionSPC id){
  m_con_status.erase(id);
  UpdateDisplayed();
}

void RunControlModel::SetStatus(eudaq::ConnectionSPC id,
                                eudaq::StatusSPC status){
  m_con_status.at(id) = status;  
  UpdateDisplayed();
}

void RunControlModel::UpdateDisplayed() {
  // if (m_disp.size() > 0) {
    // std::sort(m_disp.begin(), m_disp.end(), m_sorter);
    emit dataChanged(createIndex(0, 0), createIndex(m_con_status.size() - 1, 0));
  // }
}

bool RunControlModel::CheckConfigured() {
  return true;
}

int RunControlModel::rowCount(const QModelIndex & /*parent*/) const {
  return m_con_status.size();
}

int RunControlModel::columnCount(const QModelIndex & /*parent*/) const {
  return m_str_header.size();
}

QVariant RunControlModel::data(const QModelIndex &index, int role) const {
  if (role != Qt::DisplayRole || !index.isValid())
    return QVariant();

  auto col_n = index.column();
  auto row_n = index.row();
  
  if(row_n<m_con_status.size() && col_n<m_str_header.size()){
    auto it = m_con_status.begin();
    for(size_t i = 0; i< row_n; i++){
      it++;
    }
    auto &con = it->first;
    auto &sta = it->second;
    switch(col_n){
    case 0:
      return con->GetType().c_str();
    case 1:
      return con->GetName().c_str();
    case 2:{
      if(sta)
	return con.unique()?"DEAD":to_string(*sta).c_str();
      return "NONE_STATUS";
    }
    case 3:
      return con->GetRemote().c_str();
    default:
      return "";
    }
  }
  else
    return QVariant();
}

QVariant RunControlModel::headerData(int section, Qt::Orientation orientation,
                                     int role) const{
  if (role != Qt::DisplayRole)
    return QVariant();

  if (orientation == Qt::Horizontal && section < m_str_header.size()){
    return m_str_header[section];
  }
  return QVariant();
}

void RunControlModel::sort(int column, Qt::SortOrder order) {
  // m_sorter.SetSort(column, order == Qt::AscendingOrder);
  // UpdateDisplayed();
}
