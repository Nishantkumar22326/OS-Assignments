#include <bits/stdc++.h>
using namespace std;
#define ll long long
#define usewithum m.reserve(1<<10);m.max_load_factor(0.25);
void build (int *arr , int *tree , int l , int r , int i) {
    if (l == r) {
        tree[i] = arr[l] ;
        return ;
    }
    int m = (l+r) / 2 ;
    build (arr , tree , l , m , 2*i) ;
    build (arr , tree , m+1 , r , 2*i+1) ;
    tree[i] = min (tree[2*i] , tree[2*i+1]) ;
}
int query(int *tree , int l , int r , int i , int a , int b) {
    if (l > b || r < a) {
        return INT_MAX ;
    }
    if (a <= l && b >= r) return tree[i] ;
    int m = (l+r) / 2 ;
    return min (query (tree , l , m , 2*i , a , b) , query (tree , m+1 , r , 2*i+1 , a , b)) ;
}
void update (int *arr , int *tree , int l , int r , int i , int j , int v) {
    if (l == r) {
        tree [i] = v ;
        arr[l] = v ;
        return ;
    }
        int m = (l+r) / 2 ;
    if (j > m) {
        update (arr , tree ,m+1 , r , 2*i+1 , j , v) ;
    }
    else {
        update (arr , tree , l , m , 2*i , j , v) ;
        
    }
    tree [i] = min (tree[2*i] , tree [2*i+1]) ;
}
bool solve (int n) {
    for(int i = 2;i<=sqrt(n);i++) if (n%i == 0) return false ;
    return true ;
}
/*..................................................................................................................*/


int main(){
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);
    int t = 1;
    cin >> t;

    while(t--){
        int n; cin >> n;
        unordered_map<int ,bool> m;
        usewithum
        // cout << "2\n" ;
        for (int i = 1; i <= n; i += 2) {
            int j = i ;
            // cout << i << endl ;
            if (m[j]) continue;
            while (j <= n) {
                if (m[j] == false)
                cout << j << " " ;
                m[j] = true ;
                j = j << 1 ;
            }
        }
        cout << endl ;
    }
    return 0;
}