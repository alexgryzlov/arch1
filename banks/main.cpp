#include <iostream>

#include <assert.h>

#include "account.h"
#include "client.h"
#include "time.h"
#include "bank.h"


int main() {
    Bank bank;
    size_t my_id = bank.AddClient({"name", "surname", {}, {}, PrivilegeLevel::INITIAL});
    size_t acc_id = bank.AddAccount(my_id, std::make_unique<DebitAccount>(0, 0.0001));

    assert(bank.Withdraw({my_id, acc_id}, 1'000.) == 0);
    std::cout << bank.Deposit({my_id, acc_id}, 1'000.) << std::endl;

    std::cout << bank.GetBalanceAfter({my_id, acc_id}, 60) << std::endl;
    return 0;
}
