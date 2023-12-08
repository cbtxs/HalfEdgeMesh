#ifndef _HALFEDGE_MESH_BASE_
#define _HALFEDGE_MESH_BASE_

#include <memory>
#include <tuple>

#include "entity.h"
#include "data_container.h"
#include "tuple_type_index.h"
#include "geometry.h"

namespace HEM
{
class HalfEdgeMeshBase
{
public:
  template<typename T>
  using Array = DataContainer::DataArray<T>;
  using ENTITY = std::tuple<Node, Edge, Cell, HalfEdge>;
  using Container = std::tuple<std::shared_ptr<EntityDataContainer<Node>> & , 
                               std::shared_ptr<EntityDataContainer<Edge>> & ,
                               std::shared_ptr<EntityDataContainer<Cell>> & ,
                               std::shared_ptr<EntityDataContainer<HalfEdge>>&  >;
public:
  HalfEdgeMeshBase(): 
    node_data_(std::make_shared<EntityDataContainer<Node> >()),
    edge_data_(std::make_shared<EntityDataContainer<Edge> >()),
    cell_data_(std::make_shared<EntityDataContainer<Cell> >()),
    halfedge_data_(std::make_shared<EntityDataContainer<HalfEdge> >()),
    entity_data_(node_data_, edge_data_, cell_data_, halfedge_data_) 
  {
    node_data_->add_data<Point>("coordinate");
    halfedge_data_->add_data<uint32_t>("next");
    halfedge_data_->add_data<uint32_t>("previous");
    halfedge_data_->add_data<uint32_t>("opposite");
    halfedge_data_->add_data<uint32_t>("cell");
    halfedge_data_->add_data<uint32_t>("edge");
    halfedge_data_->add_data<uint32_t>("node");
  }

  /** 复制构造函数 */
  HalfEdgeMeshBase(const HalfEdgeMeshBase & mesh);

  /** 以单元为中心的网格 */
  HalfEdgeMeshBase(double * node, uint32_t * cell, uint32_t NN, uint32_t NC, uint32_t NV);

  template<typename Entity>
  std::shared_ptr<Array<Entity> > & get_entity()
  {
    auto * entity_data = std::get<tuple_type_index<Entity, ENTITY>>(entity_data_);
    return entity_data->get_entiy();
  }

  template<typename Entity>
  void delete_entity(Entity * e)
  {
    auto * entity_data = std::get<tuple_type_index<Entity, ENTITY>>(entity_data_);
    entity_data->delete_entity(e);
  }

  template<typename Entity>
  Entity & add_entity()
  {
    auto * entity_data = std::get<tuple_type_index<Entity, ENTITY>>(entity_data_);
    return entity_data->add_entity();
  }

  template<typename Entity>
  uint32_t number_of_entity()
  {
    auto * entity_data = std::get<tuple_type_index<Entity, ENTITY>>(entity_data_);
    return entity_data->number_of_data();
  }

  uint32_t number_of_boundary_edges() 
  { 
    uint32_t NE = edge_data_->number_of_data();
    uint32_t NHE = halfedge_data_->number_of_data();
    return NE*2-NHE;
  }

  /** 半边数据的接口 */
  Array<uint32_t> & next_halfedge() 
  {
    return *(halfedge_data_->get_data<uint32_t>("next"));
  }

  Array<uint32_t> & prev_halfedge() 
  {
    return *(halfedge_data_->get_data<uint32_t>("previous"));
  }

  Array<uint32_t> & oppo_halfedge()
  {
    return *(halfedge_data_->get_data<uint32_t>("opposite"));
  }

  Array<uint32_t> & halfedge_to_cell()
  {
    return *(halfedge_data_->get_data<uint32_t>("cell"));
  }

  Array<uint32_t> & halfedge_to_edge()
  {
    return *(halfedge_data_->get_data<uint32_t>("edge"));
  }

  Array<uint32_t> & halfedge_to_node()
  {
    return *(halfedge_data_->get_data<uint32_t>("node"));
  }

  Array<Point> & node_coordinate()
  {
    return *(node_data_->get_data<Point>("coordinate"));
  }

  /** @brief 清空网格, 但实际上没有释放内存 */
  void clear()
  {
    node_data_->clear();
    edge_data_->clear();
    cell_data_->clear();
    halfedge_data_->clear();
  }

  /** @brief 清空网格并释放内存 */
  void release()
  {
    node_data_->release();
    edge_data_->release();
    cell_data_->release();
    halfedge_data_->release();
  }

  void swap(HalfEdgeMeshBase & other)
  {
    std::swap(node_data_, other.node_data_);
    std::swap(edge_data_, other.edge_data_);
    std::swap(cell_data_, other.cell_data_);
    std::swap(halfedge_data_, other.halfedge_data_);
  }

  HalfEdgeMeshBase & operator = (const HalfEdgeMeshBase & other);

private:

  /** 实体数据集合 */
  std::shared_ptr<EntityDataContainer<Node>> node_data_; 
  std::shared_ptr<EntityDataContainer<Edge>> edge_data_;
  std::shared_ptr<EntityDataContainer<Cell>> cell_data_;
  std::shared_ptr<EntityDataContainer<HalfEdge>> halfedge_data_;

  /** 所有的实体数据存在这里 */
  Container entity_data_;
};

HalfEdgeMeshBase::HalfEdgeMeshBase(const HalfEdgeMeshBase & mesh): HalfEdgeMeshBase() 
{
  (*this) = mesh;
}

HalfEdgeMeshBase & HalfEdgeMeshBase::operator = (const HalfEdgeMeshBase & other)
{
  if(this != &other)
  {
    //clear();
    *node_data_ = *other.node_data_;
    *edge_data_ = *other.edge_data_;
    *cell_data_ = *other.cell_data_;
    *halfedge_data_ = *other.halfedge_data_;

    auto & halfedge_ = halfedge_data_->get_entity();
    auto & node_ = node_data_->get_entity(); 
    for(auto it = node_.begin(); it != node_.end(); ++it)
      it->set_halfedge(&(halfedge_[it->halfedge()->index()]));

    auto & edge_ = edge_data_->get_entity(); 
    for(auto it = edge_.begin(); it != edge_.end(); ++it)
      it->set_halfedge(&(halfedge_[it->halfedge()->index()]));

    auto & cell_ = cell_data_->get_entity(); 
    for(auto it = cell_.begin(); it != cell_.end(); ++it)
      it->set_halfedge(&(halfedge_[it->halfedge()->index()]));
  }
  return *this;
}

/** 以单元为中心的网格 */
HalfEdgeMeshBase::HalfEdgeMeshBase(double * node, uint32_t * cell, 
    uint32_t NN, uint32_t NC, uint32_t NV):HalfEdgeMeshBase()
{
  auto & coordinate = node_coordinate();
  for(uint32_t i = 0; i < NN; i++)
  {
    node_data_->add_entity();
    coordinate[i] = Point(node[i*2], node[i*2+1]);
  }

  auto & h2n = halfedge_to_node();
  //auto & h2e = halfedge_to_edge();
  auto & h2c = halfedge_to_cell();
  auto & next = next_halfedge();
  //auto & prev = prev_halfedge();
  //auto & oppo = oppo_halfedge();
  for(uint32_t i = 0; i < NC; i++)
  {
    Cell & c = cell_data_->add_entity();

    HalfEdge * current;
    HalfEdge * ne = &(halfedge_data_->add_entity());
    h2n[ne->index()] = cell[i*NV];
    h2c[ne->index()] = i;
    for(uint32_t j = 1; j < NV; j++)
    {
      current = ne;
      ne = &(halfedge_data_->add_entity());
      next[current->index()] = ne->index();
      h2n[ne->index()] = cell[i*NV+j];
      h2c[ne->index()] = i;
    }
    c.set_halfedge(ne);
  }

}


}


#endif /* _HALFEDGE_MESH_BASE_ */ 
