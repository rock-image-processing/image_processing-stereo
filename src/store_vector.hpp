#ifndef __STORE_VECTOR_HPP__
#define __STORE_VECTOR_HPP__ 

/******
simple templace functions for storing/loading entire vectors. supports the class version as well as the POD version and stream version of target template storage, e.g. if target template is a basic type, use POD, if target template is a class which has the .store and .read functions implemented use class
******/

template<typename T>
void StoreClassVector(const std::vector<T>& Tvector, std::ostream& os)
{ 
  typename std::vector<T>::size_type size(Tvector.size());
  os.write(reinterpret_cast<const char*>(&size), sizeof(typename std::vector<T>::size_type));
  for(typename std::vector<T>::size_type i =0; i < size; ++i)
  {
    Tvector[i].store(os);
  }
};
  
template<typename T>
void LoadClassVector(std::vector<T>& Tvector, std::istream& is)
{ 
  typename std::vector<T>::size_type to_load_size(0);
  is.read(reinterpret_cast<char*>(&to_load_size), sizeof(typename std::vector<T>::size_type));
  Tvector.resize(to_load_size);
  for(typename std::vector<T>::size_type i =0; i < to_load_size; ++i)
  {
    Tvector[i].load(is);
  }
};
  
template<typename T>
void StorePODVector(const std::vector<T>& Tvector, std::ostream& os)
{ 
  typename std::vector<T>::size_type size(Tvector.size());
  os.write(reinterpret_cast<const char*>(&size), sizeof(typename std::vector<T>::size_type));
  for(typename std::vector<T>::size_type i = 0; i < size; i++)
    os << Tvector[i] << " ";
};
  
template<typename T>
void LoadPODVector(std::vector<T>& Tvector, std::istream& is)
{
  typename std::vector<T>::size_type to_load_size(0);
  is.read(reinterpret_cast<char*>(&to_load_size), sizeof(typename std::vector<T>::size_type));
  T temp;
  for(typename std::vector<T>::size_type i = 0; i < to_load_size; i++)
  {
    is >> temp;
    Tvector.push_back(temp);
  }
};  

#endif