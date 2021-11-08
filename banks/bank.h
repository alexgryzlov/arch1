#ifndef BANKS_BANK_H
#define BANKS_BANK_H

#include <unordered_map>
#include <vector>
#include <memory>

#include "client.h"

struct AccountDescriptor {
    size_t client_id;
    size_t account_id;
};

struct Transaction {
    size_t id;
    AccountDescriptor from;
    AccountDescriptor to;
    double sum;
};

class Bank {
public:
    Bank() :
            max_amount_({
                                {PrivilegeLevel::INITIAL,      1'000},
                                {PrivilegeLevel::INTERMEDIATE, 10'000},
                                {PrivilegeLevel::FULL, SIZE_MAX}
                        }) {
        max_amount_.at(PrivilegeLevel::INITIAL);
    }


    // throws if client's declared privilege level requires more information
    size_t AddClient(const Client &client) {
        if (!CheckClientRequirements(client)) {
            throw std::runtime_error("Not enough information for given privilege level.");
        }
        size_t id = last_client_id_++;
        clients_[id] = client;
        return id;
    }

    // throws if client does not exist
    size_t AddAccount(size_t client_id, std::unique_ptr<Account> account) {
        GetClient(client_id);

        size_t account_id = last_account_id_++;
        client_accounts_[client_id][account_id] = std::move(account);
        return account_id;
    }

    // throws if client does not exist
    void RaiseClientPrivilege(size_t id, const Client &new_client) {
        auto client = GetClient(id);
        if (new_client.privilege_level <= client.privilege_level) {
            return;
        }
        switch (new_client.privilege_level) {
            case PrivilegeLevel::FULL:
                client.passport = new_client.passport;
            case PrivilegeLevel::INTERMEDIATE:
                client.address = new_client.address;
        }
    }

    // throws if client or account does not exist
    // throws if account tries to withdraw too much
    size_t Withdraw(AccountDescriptor desc, double sum) {
        auto[client, account] = GetAccount(desc);
        if (sum > max_amount_.at(client.privilege_level)) {
            throw std::runtime_error("Withdraw amount is too big for current privilage level.");
        }
        return std::abs(account->ChangeBalance(-sum, Time::current_time()));
    }

    // throws if client or account does not exist
    size_t Deposit(AccountDescriptor desc, double sum) {
        auto[client, account] = GetAccount(desc);
        return account->ChangeBalance(sum, Time::current_time());
    }

    // throws if any client or account does not exist
    // returns 0 if from account does not have enough money. otherwise, returns transaction_id.
    size_t Transfer(AccountDescriptor from, AccountDescriptor to, double sum) {
        auto[from_client, from_account] = GetAccount(from);
        if (from_account->Has(sum, Time::current_time())) {
            Withdraw(from, sum);
            Deposit(to, sum);

            auto transaction_id = last_transaction_id_++;
            transaction_history_.push_back({transaction_id, from, to, sum});
            return transaction_id;
        }
        return 0;
    }

    void AddPercents() const {
        for (const auto&[client_id, accounts] : client_accounts_) {
            for (const auto&[account_id, account] : accounts) {
                account->AddPercent();
            }
        }
    }

    void CommitPercents() const {
        for (const auto&[client_id, accounts] : client_accounts_) {
            for (const auto&[account_id, account] : accounts) {
                account->CommitPercent();
            }
        }
    }

    // TODO: how to return unique_ptr with new object using only base class pointer?
    double GetBalanceAfter(AccountDescriptor desc, time_t duration) {
        time_t requested_at = Time::current_time();
        auto[_, account] = GetAccount(desc);

        auto imaginary_account = account->__clone();
        for (time_t time = requested_at; time < requested_at + duration; ++time) {
            imaginary_account->AddPercent();
            if (time % Time::month_duration() == 0) {
                imaginary_account->CommitPercent();
            }
        }

        auto result_balance = imaginary_account->balance();
        delete imaginary_account;
        return result_balance;
    }

    // Cancel transaction if current balances allow to do so
    void CancelTransaction(size_t transaction_id) {
        Transaction sought_transaction = {transaction_id, {}, {}, 0.};
        auto it = std::lower_bound(transaction_history_.begin(), transaction_history_.end(), sought_transaction,
                                   [](const auto &lhs, const auto &rhs) {
                                       return lhs.id < rhs.id;
                                   });
        if (it == transaction_history_.end() || it->id != transaction_id) {
            return;
        }

        sought_transaction = *it;
        {
            auto[_, from_account] = GetAccount(sought_transaction.from);
            from_account->ForceChangeBalance(from_account->balance() + sought_transaction.sum);
        }
        {
            auto[_, to_account] = GetAccount(sought_transaction.to);
            to_account->ForceChangeBalance(to_account->balance() - sought_transaction.sum);
        }
    }

private:
    bool IsSuspicious(size_t client_id) {
        return clients_[client_id].privilege_level != PrivilegeLevel::FULL;
    }

    bool CheckClientRequirements(const Client &client) {
        bool satisfies = true;
        switch (client.privilege_level) {
            case PrivilegeLevel::FULL:
                satisfies &= client.passport.has_value();
            case PrivilegeLevel::INTERMEDIATE:
                satisfies &= client.address.has_value();
            case PrivilegeLevel::INITIAL:
                break;
            default:
                satisfies = false;
        }
        return satisfies;
    }

    Client &GetClient(size_t client_id) {
        if (!clients_.contains(client_id)) {
            throw std::runtime_error("Client does not exist.");
        }
        return clients_[client_id];
    }

    std::pair<Client &, Account *> GetAccount(AccountDescriptor desc) {
        auto &client = GetClient(desc.client_id);
        if (!client_accounts_[desc.client_id].contains(desc.account_id)) {
            throw std::runtime_error("Account does not exist.");
        }
        return {client, client_accounts_[desc.client_id][desc.account_id].get()};
    }

    std::unordered_map<size_t, Client> clients_;
    std::unordered_map<size_t, std::unordered_map<size_t, std::unique_ptr<Account>>> client_accounts_;
    std::vector<Transaction> transaction_history_;
    std::unordered_map<PrivilegeLevel, size_t> max_amount_;
    size_t last_client_id_ = 1;
    size_t last_account_id_ = 1;
    size_t last_transaction_id_ = 1;
};
#endif //BANKS_BANK_H
