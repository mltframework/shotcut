// 分类管理功能
class CategoryManager {
  constructor() {
    this.categories = [];
  }

  // 添加分类
  addCategory(name, description) {
    const newCategory = {
      id: this.categories.length + 1,
      name: name,
      description: description,
      createdAt: new Date()
    };
    this.categories.push(newCategory);
    return newCategory;
  }

  // 获取分类列表
  getCategories() {
    return this.categories;
  }

  // 搜索分类
  searchCategories(keyword) {
    return this.categories.filter(category => 
      category.name.includes(keyword) || 
      category.description.includes(keyword)
    );
  }
}

module.exports = CategoryManager;
